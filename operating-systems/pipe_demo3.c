#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define MESSAGE_SIZE 256

void demonstrate_basic_pipe() {
    printf("=== Basic Parent-Child Pipe Communication ===\n");
    
    int fd[2], cpid;
    char write_buffer[] = "Hello from parent process!";
    char read_buffer[BUFFER_SIZE];
    
    // Create pipe
    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(1);
    }
    
    printf("Parent: Created pipe (read_fd=%d, write_fd=%d)\n", fd[0], fd[1]);
    
    switch (cpid = fork()) {
        case -1:
            perror("fork failed");
            exit(1);
            
        case 0:  /* child process */
            printf("Child: Process started (PID=%d)\n", getpid());
            
            // Child closes read end
            close(fd[0]);
            printf("Child: Closed read end of pipe\n");
            
            // Child writes to pipe
            printf("Child: Writing message to pipe: '%s'\n", write_buffer);
            if (write(fd[1], write_buffer, strlen(write_buffer) + 1) == -1) {
                perror("write failed");
                exit(1);
            }
            
            // Child closes write end
            close(fd[1]);
            printf("Child: Closed write end of pipe and exiting\n");
            exit(0);
            
        default: /* parent process */
            printf("Parent: Forked child process (PID=%d)\n", cpid);
            
            // Parent closes write end
            close(fd[1]);
            printf("Parent: Closed write end of pipe\n");
            
            // Parent reads from pipe
            printf("Parent: Waiting to read from pipe...\n");
            ssize_t bytes_read = read(fd[0], read_buffer, BUFFER_SIZE - 1);
            if (bytes_read == -1) {
                perror("read failed");
                exit(1);
            }
            
            read_buffer[bytes_read] = '\0';
            printf("Parent: Read %zd bytes: '%s'\n", bytes_read, read_buffer);
            
            // Parent closes read end
            close(fd[0]);
            
            // Wait for child to complete
            int status;
            wait(&status);
            printf("Parent: Child process completed with status %d\n", status);
    }
    
    printf("\n");
}

void demonstrate_bidirectional_communication() {
    printf("=== Bidirectional Communication (Two Pipes) ===\n");
    
    int pipe1[2], pipe2[2], cpid;  // pipe1: parent->child, pipe2: child->parent
    char parent_msg[] = "Parent: What's your favorite number?";
    char child_msg[] = "Child: My favorite number is 42!";
    char buffer[BUFFER_SIZE];
    
    // Create two pipes
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe creation failed");
        exit(1);
    }
    
    printf("Parent: Created two pipes for bidirectional communication\n");
    
    switch (cpid = fork()) {
        case -1:
            perror("fork failed");
            exit(1);
            
        case 0:  /* child process */
            printf("Child: Process started\n");
            
            // Child closes unused ends
            close(pipe1[1]); // Close write end of pipe1 (parent->child)
            close(pipe2[0]); // Close read end of pipe2 (child->parent)
            
            // Child reads message from parent
            read(pipe1[0], buffer, BUFFER_SIZE);
            printf("Child: Received from parent: '%s'\n", buffer);
            
            // Child sends response to parent
            printf("Child: Sending response to parent\n");
            write(pipe2[1], child_msg, strlen(child_msg) + 1);
            
            // Clean up
            close(pipe1[0]);
            close(pipe2[1]);
            exit(0);
            
        default: /* parent process */
            printf("Parent: Forked child process\n");
            
            // Parent closes unused ends
            close(pipe1[0]); // Close read end of pipe1 (parent->child)
            close(pipe2[1]); // Close write end of pipe2 (child->parent)
            
            // Parent sends message to child
            printf("Parent: Sending message to child\n");
            write(pipe1[1], parent_msg, strlen(parent_msg) + 1);
            
            // Parent reads response from child
            read(pipe2[0], buffer, BUFFER_SIZE);
            printf("Parent: Received from child: '%s'\n", buffer);
            
            // Clean up
            close(pipe1[1]);
            close(pipe2[0]);
            
            wait(NULL);
            printf("Parent: Communication complete\n");
    }
    
    printf("\n");
}

void demonstrate_multiple_messages() {
    printf("=== Multiple Messages Through Single Pipe ===\n");
    
    int fd[2], cpid;
    char messages[][MESSAGE_SIZE] = {
        "Message 1: Hello World!",
        "Message 2: Unix pipes are awesome!",
        "Message 3: This is the final message."
    };
    int num_messages = sizeof(messages) / sizeof(messages[0]);
    char buffer[BUFFER_SIZE];
    
    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(1);
    }
    
    switch (cpid = fork()) {
        case -1:
            perror("fork failed");
            exit(1);
            
        case 0:  /* child process - writer */
            close(fd[0]); // Close read end
            
            printf("Child: Sending %d messages...\n", num_messages);
            for (int i = 0; i < num_messages; i++) {
                printf("Child: Sending message %d: '%s'\n", i + 1, messages[i]);
                write(fd[1], messages[i], strlen(messages[i]) + 1);
                sleep(1); // Small delay between messages
            }
            
            close(fd[1]);
            printf("Child: All messages sent\n");
            exit(0);
            
        default: /* parent process - reader */
            close(fd[1]); // Close write end
            
            printf("Parent: Ready to receive messages...\n");
            for (int i = 0; i < num_messages; i++) {
                ssize_t bytes_read = read(fd[0], buffer, BUFFER_SIZE);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    printf("Parent: Received message %d: '%s'\n", i + 1, buffer);
                }
            }
            
            close(fd[0]);
            wait(NULL);
            printf("Parent: All messages received\n");
    }
    
    printf("\n");
}

void demonstrate_pipe_capacity() {
    printf("=== Pipe Buffer Capacity Demonstration ===\n");
    
    int fd[2];
    char data[1024];  // Write in 1KB chunks for efficiency
    memset(data, 'A', sizeof(data));
    int total_written = 0;
    
    if (pipe(fd) == -1) {
        perror("pipe failed");
        exit(1);
    }
    
    printf("Parent: Created pipe, now filling buffer without any reader\n");
    printf("Parent: No child process - pipe will fill up and block\n");
    
    // Make write non-blocking so we can detect when it would block
    int flags = fcntl(fd[1], F_GETFL);
    fcntl(fd[1], F_SETFL, flags | O_NONBLOCK);
    
    // Fill the pipe buffer
    while (1) {
        ssize_t bytes_written = write(fd[1], data, sizeof(data));
        
        if (bytes_written > 0) {
            total_written += bytes_written;
            if (total_written % (8 * 1024) == 0) {  // Report every 8KB
                printf("Parent: Written %d bytes (%.1f KB) so far\n", 
                       total_written, total_written / 1024.0);
            }
        } else if (bytes_written == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Parent: Pipe buffer is now FULL!\n");
                printf("Parent: Total capacity: %d bytes (%.1f KB)\n", 
                       total_written, total_written / 1024.0);
                break;
            } else {
                perror("write error");
                break;
            }
        }
        
        // Safety check
        if (total_written > 10 * 1024 * 1024) {  // 10MB safety limit
            printf("Parent: Reached 10MB safety limit - something's wrong\n");
            break;
        }
    }
    
    printf("Parent: Pipe buffer demonstration complete\n");
    printf("Parent: Now creating child to drain the pipe...\n");
    
    // Now fork a child to read the data
    pid_t cpid = fork();
    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);
            
        case 0:  /* child process */
            close(fd[1]); // Close write end
            
            printf("Child: Starting to drain the pipe buffer\n");
            char read_buffer[1024];
            int total_read = 0;
            ssize_t bytes_read;
            
            while ((bytes_read = read(fd[0], read_buffer, sizeof(read_buffer))) > 0) {
                total_read += bytes_read;
            }
            
            printf("Child: Drained %d bytes (%.1f KB) from pipe\n", 
                   total_read, total_read / 1024.0);
            close(fd[0]);
            exit(0);
            
        default: /* parent process */
            close(fd[1]); // Close write end
            wait(NULL);   // Wait for child to finish
            printf("Parent: Child finished draining pipe\n");
    }
    
    close(fd[0]);
    printf("\n");
}

int main() {
    printf("Unix Pipe Demonstration Program\n");
    printf("================================\n\n");
    
    // Demonstrate basic pipe communication
    demonstrate_basic_pipe();
    
    // Demonstrate bidirectional communication
    demonstrate_bidirectional_communication();
    
    // Demonstrate multiple messages
    demonstrate_multiple_messages();
    
    // Demonstrate pipe buffer capacity
    demonstrate_pipe_capacity();
    
    printf("All demonstrations completed successfully!\n");
    printf("\nTo compile: gcc -o pipe_demo pipe_demo.c\n");
    printf("To run: ./pipe_demo\n");
    
    return 0;
}
