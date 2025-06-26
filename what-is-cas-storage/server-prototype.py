import os
import hashlib
import shutil
import logging
from pathlib import Path
from typing import List, Dict, Set, Optional
import json
import time

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class Storage:
    def __init__(self):
        self.data_dir: Optional[Path] = None
        self.metadata_dir: Optional[Path] = None
        self.hash_to_paths: Dict[str, Set[str]] = {}  # hash -> set of original file paths
        self.path_to_hash: Dict[str, str] = {}  # original path -> hash
        self.is_initialized = False
    
    @classmethod
    def init(cls, data_dir: str) -> 'Storage':
        """Initialize CAS storage to use the given file path as underlying storage."""
        storage = cls()
        storage.data_dir = Path(data_dir)
        storage.metadata_dir = storage.data_dir / "metadata"
        
        # Create directories
        storage.data_dir.mkdir(parents=True, exist_ok=True)
        storage.metadata_dir.mkdir(parents=True, exist_ok=True)
        
        storage.is_initialized = True
        
        # Load existing metadata into memory
        storage._load_existing_metadata()
        
        logger.info(f"Initialized CAS storage at: {storage.data_dir}")
        logger.info(f"Metadata directory at: {storage.metadata_dir}")
        logger.info(f"Loaded {len(storage.hash_to_paths)} existing hashes from metadata")
        return storage
    
    def _load_existing_metadata(self) -> None:
        """Load all existing metadata files into memory for duplicate detection."""
        if not self.metadata_dir.exists():
            return
        
        for metadata_file in self.metadata_dir.glob("*.json"):
            try:
                with open(metadata_file, 'r', encoding='utf-8') as f:
                    metadata = json.load(f)
                    
                file_hash = metadata.get("hash")
                original_path = metadata.get("original_path")
                aliases = metadata.get("aliases", [])
                
                if not file_hash:
                    continue
                
                # Initialize the hash entry if it doesn't exist
                if file_hash not in self.hash_to_paths:
                    self.hash_to_paths[file_hash] = set()
                
                # Add original path if it exists
                if original_path:
                    self.hash_to_paths[file_hash].add(original_path)
                    self.path_to_hash[original_path] = file_hash
                
                # Add all aliases
                for alias_path in aliases:
                    self.hash_to_paths[file_hash].add(alias_path)
                    self.path_to_hash[alias_path] = file_hash
                    
            except Exception as e:
                logger.error(f"Error loading metadata from {metadata_file}: {e}")
                continue
    
    def _get_metadata_file_path(self, file_hash: str) -> Path:
        """Get the metadata file path for a given hash."""
        return self.metadata_dir / f"{file_hash}.json"
    
    def _load_metadata(self, file_hash: str) -> Dict:
        """Load metadata for a given hash from disk."""
        metadata_path = self._get_metadata_file_path(file_hash)
        if metadata_path.exists():
            try:
                with open(metadata_path, 'r', encoding='utf-8') as f:
                    metadata = json.load(f)
                    # Ensure aliases list exists
                    if "aliases" not in metadata:
                        metadata["aliases"] = []
                    return metadata
            except Exception as e:
                logger.error(f"Error loading metadata for hash {file_hash}: {e}")
        
        return {
            "hash": file_hash,
            "original_path": None,
            "stored_path": None,
            "aliases": [],
            "created_at": None,
            "file_size": None
        }
    
    def _save_metadata(self, file_hash: str, metadata: Dict) -> None:
        """Save metadata for a given hash to disk."""
        metadata_path = self._get_metadata_file_path(file_hash)
        try:
            # Ensure the metadata directory exists
            metadata_path.parent.mkdir(parents=True, exist_ok=True)
            
            with open(metadata_path, 'w', encoding='utf-8') as f:
                json.dump(metadata, f, indent=2, ensure_ascii=False)
            logger.debug(f"Saved metadata for hash {file_hash[:8]}... to {metadata_path}")
        except Exception as e:
            logger.error(f"Error saving metadata for hash {file_hash}: {e}")
            raise
    
    def _update_metadata_for_file(self, file_path: str, file_hash: str, stored_path: str, is_duplicate: bool = False) -> None:
        """Update metadata when a file is added to storage."""
        metadata = self._load_metadata(file_hash)
        
        if not is_duplicate:
            # This is the first occurrence of this content
            metadata.update({
                "hash": file_hash,
                "original_path": file_path,
                "stored_path": stored_path,
                "created_at": time.time(),
                "file_size": os.path.getsize(file_path)
            })
            # Initialize aliases list if not present
            if "aliases" not in metadata:
                metadata["aliases"] = []
            logger.debug(f"Created metadata for original file: {file_path}")
        else:
            # This is a duplicate - add as alias
            if "aliases" not in metadata:
                metadata["aliases"] = []
            
            if file_path not in metadata["aliases"]:
                metadata["aliases"].append(file_path)
                logger.info(f"Added alias to metadata for hash {file_hash[:8]}...: {file_path}")
                logger.debug(f"Current aliases for {file_hash[:8]}...: {metadata['aliases']}")
            else:
                logger.debug(f"Alias {file_path} already exists for hash {file_hash[:8]}...")
        
        self._save_metadata(file_hash, metadata)
 
    def _calculate_file_hash(self, file_path: str) -> str:
        """Calculate SHA-256 hash of file content."""
        hash_sha256 = hashlib.sha256()
        try:
            with open(file_path, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hash_sha256.update(chunk)
            return hash_sha256.hexdigest()
        except Exception as e:
            logger.error(f"Error calculating hash for {file_path}: {e}")
            raise
 
    def _store_file(self, file_path: str, file_hash: str) -> str:
        """Store file in CAS storage using its hash as the key."""
        if not self.is_initialized:
            raise RuntimeError("Storage not initialized. Call Storage.init() first.")
        
        # Create subdirectories based on first few characters of hash for better distribution
        subdir = self.data_dir / "content" / file_hash[:2] / file_hash[2:4]
        subdir.mkdir(parents=True, exist_ok=True)
        
        stored_path = subdir / f"{file_hash}.pdf"
        
        # Only copy if file doesn't already exist in storage
        if not stored_path.exists():
            shutil.copy2(file_path, stored_path)
            logger.debug(f"Stored file {file_path} as {stored_path}")
        
        return str(stored_path)
    
    def addAll(self, load_dir: str, scan_recursively: bool = True) -> None:
        """
        Scan all files and add all PDF files in load_dir to storage.
        Generate CAS keys and report duplicates.
        """
        if not self.is_initialized:
            raise RuntimeError("Storage not initialized. Call Storage.init() first.")
        
        load_path = Path(load_dir)
        if not load_path.exists():
            raise FileNotFoundError(f"Load directory does not exist: {load_dir}")
        
        # Find all PDF files
        if scan_recursively:
            pdf_files = list(load_path.rglob("*.pdf"))
        else:
            pdf_files = list(load_path.glob("*.pdf"))
        
        logger.info(f"Found {len(pdf_files)} PDF files to process in {load_dir}")
        
        processed_count = 0
        duplicate_count = 0
        
        for pdf_file in pdf_files:
            try:
                file_path = str(pdf_file.absolute())
                file_hash = self._calculate_file_hash(file_path)
                
                # Check if this hash already exists (from previous addAll calls or current session)
                if file_hash in self.hash_to_paths:
                    # This is a duplicate
                    existing_paths = list(self.hash_to_paths[file_hash])
                    logger.info(f"DUPLICATE FOUND: {file_path} is duplicate of {existing_paths[0]}")
                    duplicate_count += 1
                    
                    # Load existing metadata to get stored path
                    metadata = self._load_metadata(file_hash)
                    stored_path = metadata.get("stored_path", "")
                    
                    # Update metadata with alias
                    self._update_metadata_for_file(file_path, file_hash, stored_path, is_duplicate=True)
                else:
                    # Store the file in CAS storage (first occurrence)
                    stored_path = self._store_file(file_path, file_hash)
                    self.hash_to_paths[file_hash] = set()
                    
                    # Update metadata for original file
                    self._update_metadata_for_file(file_path, file_hash, stored_path, is_duplicate=False)
                
                # Track the mapping regardless of whether it's a duplicate
                self.hash_to_paths[file_hash].add(file_path)
                self.path_to_hash[file_path] = file_hash
                processed_count += 1
                
            except Exception as e:
                logger.error(f"Error processing file {pdf_file}: {e}")
                continue
        
        logger.info(f"Processing complete for {load_dir}. Processed: {processed_count}, Duplicates found: {duplicate_count}")
    
    def fetchDuplicates(self, file_path: str) -> List[str]:
        """Fetch all known duplicates of file at file_path."""
        if not self.is_initialized:
            raise RuntimeError("Storage not initialized. Call Storage.init() first.")
        
        abs_path = str(Path(file_path).absolute())
        
        if abs_path not in self.path_to_hash:
            logger.warning(f"File {file_path} not found in storage metadata")
            return []
        
        file_hash = self.path_to_hash[abs_path]
        
        # Load metadata to get all aliases
        metadata = self._load_metadata(file_hash)
        all_paths = []
        
        # Add original path if it exists
        if metadata.get("original_path"):
            all_paths.append(metadata["original_path"])
        
        # Add all aliases
        all_paths.extend(metadata.get("aliases", []))
        
        # Return all paths except the queried one
        duplicates = [path for path in all_paths if path != abs_path]
        return duplicates
    
    def getStats(self) -> Dict[str, str]:
        """Report total bytes of storage in actual bytes and human readable format."""
        if not self.is_initialized:
            raise RuntimeError("Storage not initialized. Call Storage.init() first.")
        
        total_bytes = 0
        file_count = 0
        total_aliases = 0
        
        # Calculate total size of stored files and count aliases
        content_dir = self.data_dir / "content"
        if content_dir.exists():
            for root, dirs, files in os.walk(content_dir):
                for file in files:
                    if file.endswith('.pdf'):
                        file_path = Path(root) / file
                        try:
                            total_bytes += file_path.stat().st_size
                            file_count += 1
                        except OSError:
                            continue
        
        # Count total aliases from metadata
        for metadata_file in self.metadata_dir.glob("*.json"):
            try:
                with open(metadata_file, 'r', encoding='utf-8') as f:
                    metadata = json.load(f)
                    total_aliases += len(metadata.get("aliases", []))
            except Exception:
                continue
        
        def human_readable_size(size_bytes: int) -> str:
            """Convert bytes to human readable format."""
            if size_bytes == 0:
                return "0 B"
            
            size_names = ["B", "KB", "MB", "GB", "TB"]
            i = 0
            size = float(size_bytes)
            
            while size >= 1024.0 and i < len(size_names) - 1:
                size /= 1024.0
                i += 1
            
            return f"{size:.2f} {size_names[i]}"
        
        return {
            "total_bytes": str(total_bytes),
            "human_readable": human_readable_size(total_bytes),
            "unique_files": str(file_count),
            "total_aliases": str(total_aliases),
            "unique_hashes": str(len(self.hash_to_paths)),
            "total_logical_files": str(file_count + total_aliases)
        }
    
    def debugMetadata(self, file_hash: str = None) -> None:
        """Debug method to print metadata information."""
        if not self.is_initialized:
            raise RuntimeError("Storage not initialized. Call Storage.init() first.")
        
        if file_hash:
            # Debug specific hash
            metadata = self._load_metadata(file_hash)
            print(f"Metadata for hash {file_hash[:8]}...:")
            print(f"  Original: {metadata.get('original_path', 'None')}")
            print(f"  Stored: {metadata.get('stored_path', 'None')}")
            print(f"  Aliases: {metadata.get('aliases', [])}")
            print(f"  Aliases count: {len(metadata.get('aliases', []))}")
        else:
            # Debug all metadata
            print("All metadata files:")
            for metadata_file in self.metadata_dir.glob("*.json"):
                try:
                    with open(metadata_file, 'r', encoding='utf-8') as f:
                        metadata = json.load(f)
                        hash_val = metadata.get('hash', 'unknown')
                        print(f"  {hash_val[:8]}...: {len(metadata.get('aliases', []))} aliases")
                except Exception as e:
                    print(f"  Error reading {metadata_file}: {e}")
    
    def getMetadata(self, file_path: str) -> Optional[Dict]:
        """Get metadata for a specific file."""
        if not self.is_initialized:
            raise RuntimeError("Storage not initialized. Call Storage.init() first.")
        
        abs_path = str(Path(file_path).absolute())
        
        if abs_path not in self.path_to_hash:
            return None
        
        file_hash = self.path_to_hash[abs_path]
        return self._load_metadata(file_hash)
    
    def listAllFiles(self) -> List[Dict]:
        """List all files in storage with their metadata."""
        if not self.is_initialized:
            raise RuntimeError("Storage not initialized. Call Storage.init() first.")
        
        all_files = []
        
        for metadata_file in self.metadata_dir.glob("*.json"):
            try:
                with open(metadata_file, 'r', encoding='utf-8') as f:
                    metadata = json.load(f)
                    all_files.append(metadata)
            except Exception as e:
                logger.error(f"Error reading metadata file {metadata_file}: {e}")
                continue
        
        return all_files

# Example usage and testing
if __name__ == "__main__":
    # Example usage with debugging
    storage = Storage.init("./cas_storage5")
    
    # Add files from multiple directories to test cross-directory duplicate detection
    #storage.addAll("./load1", scan_recursively=True)
    storage.addAll("./load11", scan_recursively=True)
    
    # Debug all metadata
    # storage.debugMetadata()
    
    # Get statistics
    stats = storage.getStats()
    print("Storage Statistics:")
    for key, value in stats.items():
        print(f"  {key}: {value}")
    
    # Example of checking duplicates
    # duplicates = storage.fetchDuplicates("./load1/influx.pdf")
    # print(f"Duplicates: {duplicates}")
    
    # Example of getting metadata
    metadata = storage.getMetadata("./load1/influx.pdf")
    if metadata:
        print(f"Original: {metadata['original_path']}")
        print(f"Stored: {metadata['stored_path']}")
        print(f"Aliases: {metadata['aliases']}")
        # Debug specific metadata
        storage.debugMetadata(metadata['hash'])
    
    # List all files
    # all_files = storage.listAllFiles()
    # print(f"Total files tracked: {len(all_files)}")
