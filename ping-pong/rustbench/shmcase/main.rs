use memmap2::{MmapMut, MmapOptions};
use std::env;
use std::fs::{File, OpenOptions};
use std::io::{Write};
use std::path::Path;
use std::time::{Duration, Instant};

const FILE_PATH: &str = "/tmp/pingpong-shm.dat";
const REPORT_INTERVAL: usize = 10_000;
const MAP_SIZE: usize = 1;

fn main() -> std::io::Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 || (args[1] != "ping" && args[1] != "pong") {
        eprintln!("Usage: {} <ping|pong>", args[0]);
        std::process::exit(1);
    }

    let is_ping = args[1] == "ping";
    let file_exists = Path::new(FILE_PATH).exists();

    if is_ping && !file_exists {
        let mut file = File::create(FILE_PATH)?;
        file.write_all(&[0u8])?;
        file.set_len(MAP_SIZE as u64)?;
    }

    let file = OpenOptions::new()
        .read(true)
        .write(true)
        .create(true)
        .open(FILE_PATH)?;

    let mut mmap = unsafe { MmapOptions::new().len(MAP_SIZE).map_mut(&file)? };

    let mut counter: usize = 0;
    let start = Instant::now();

    loop {
        let turn = mmap[0];

        if is_ping && turn == 0 {
            mmap[0] = 1;
            counter += 1;
        } else if !is_ping && turn == 1 {
            mmap[0] = 0;
            counter += 1;
        }

        if counter % REPORT_INTERVAL == 0 && counter > 0 {
            let elapsed = start.elapsed();
            let latency_us = elapsed.as_micros() as f64 / counter as f64;
            let throughput = counter as f64 / elapsed.as_secs_f64();
            println!(
                "{}: {} rounds, Avg latency: {:.2} µs, Throughput: {:.2} rounds/sec",
                if is_ping { "PING" } else { "PONG" },
                counter, latency_us, throughput
            );
        }
    }
}
