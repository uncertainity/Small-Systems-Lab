#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <array>
#include <cstring>

using namespace std;

constexpr size_t BLOCK_SIZE = 4096;

constexpr size_t DISK_SIZE = 512ULL * 1024 * 1024;
constexpr size_t NUM_BLOCKS = DISK_SIZE / BLOCK_SIZE;


int create_disk(const char* filename, const size_t disk_size){
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if(fd == -1){
        perror("open");
        return -1;
    }
    if(ftruncate(fd,disk_size) == -1){
        perror("Truncate");
        close(fd);
        return -1;
    }
    return fd;
}


int write_in_disk(int fd, char* data, size_t block_id){
    if(block_id >= NUM_BLOCKS){
        cerr << "Invalid Block ID\n";
        return 1;
    }
    off_t offset = block_id * BLOCK_SIZE;
    ssize_t n = pwrite(fd,data,BLOCK_SIZE,offset);
    if(n == -1){
        perror("writing failed");
        return 1;
    }
    else if((size_t)n < BLOCK_SIZE){
        cerr << "Partial Write.\n";
        return 1;
    }
    else{
        cout<<"Block successfully written."<<endl;
    }
    return 0;
}

int robust_write_in_disk(int fd, char* data,size_t block_id){
    if(block_id >= NUM_BLOCKS){
        cerr<<"Invalid Block ID"<<endl;
        return 1;
    }
    size_t written = 0;
    off_t offset = block_id * BLOCK_SIZE;
    while(written < BLOCK_SIZE){
        ssize_t n = pwrite(fd,data,BLOCK_SIZE - written,offset);
        if(n == -1){
            if(errno == EINTR){
                continue;
            }
            perror("pwrite error");
            return 1;
        }
        else if(n == 0){
            cerr << "Write made no progress\n";
            return 1;
        }

        written += (size_t)n;
        data = data + n;
        offset += n;
    }
    if (written == BLOCK_SIZE){
        cout<<"Block successfully written."<<endl;
    }
    return 0;
}

ssize_t read_in_disk(int fd, char* buffer,size_t block_id){

    off_t offset = block_id * BLOCK_SIZE;
    ssize_t n = pread(fd,buffer,BLOCK_SIZE,offset);
    if(n == -1){
        perror("reading failed");
        return -1;
    }
    else if((size_t)n < BLOCK_SIZE){
        cout<<"Partial Read." << endl;
    }
    else{
        cout<<"Block successfully read."<<endl;
    }
    return n;
}

ssize_t robust_read_in_disk(int fd, char* buffer, size_t block_id){
    size_t bytes_read = 0;
    off_t offset = block_id * BLOCK_SIZE;
    while(bytes_read < BLOCK_SIZE){
        ssize_t n = pread(fd,buffer + bytes_read,BLOCK_SIZE - bytes_read,offset);
        if(n == -1){
            if(errno == EINTR){
                continue;
            }
            perror("pread error.");
            return -1;
        }
        else if(n == 0){
            cerr<<"Read made no progress. \n";
            return -1;
        }
        bytes_read += (size_t)n; 
        offset += n;
    }
    if(bytes_read == BLOCK_SIZE){
        cout<<"Block successfully read."<<endl;
    }

    return BLOCK_SIZE;
}


void verify_persistence(const char* filename, int num_writes){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(0,NUM_BLOCKS - 1);
    uniform_int_distribution<int> char_dist(0,255);
    uniform_int_distribution<int> char_length(0,BLOCK_SIZE);
    unordered_map<size_t,array<char,BLOCK_SIZE>> expected;
    
    int fd;

    fd = open(filename,O_RDWR);
    if (fd == -1) {
        perror("open");
        return;
    }

    for(int i = 0; i < num_writes; i++){
        int block_id = dist(gen);
        int length = char_length(gen);
        array<char, BLOCK_SIZE> block_bytes{};
        for(int j = 0; j < length; j++){
            block_bytes[j] = static_cast<char>(char_dist(gen));
        }
        int write_status = robust_write_in_disk(fd,block_bytes.data(),block_id);
        if(write_status == 0){
            expected[block_id] = block_bytes;
        }
        else{
            cerr << "Write failed for block " << block_id << "\n";
        }
        
    }
    
    if (fd == -1) {
        perror("open");
        return;
    }
    close(fd);

    fd = open(filename,O_RDWR);
    int equal_count = 0;
    
    for(const auto& [block_id,block_bytes] : expected){
        char read_buffer[BLOCK_SIZE];
        ssize_t read_status = robust_read_in_disk(fd,read_buffer,block_id);
        if(read_status == -1){
            perror("Read Failed..");
            continue;
        }
        if(read_status == BLOCK_SIZE && memcmp(read_buffer,block_bytes.data(),BLOCK_SIZE) == 0){
            equal_count ++;
        }
        else{
            cout<<block_id << "did not match."<<endl; 
        }
    } 
    cout << "Verified " << equal_count
    << " / " << expected.size()
    << " unique blocks.\n";

    if (fsync(fd) == -1) {
        perror("fsync");
        close(fd);
        return;
    }

    close(fd);

}

int main(){
    //constexpr size_t DISK_SIZE = 512ULL * 1024 * 1024;
    //static size_t NUM_BLOCKS = DISK_SIZE/ BLOCK_SIZE;
    const char* filename = "disk.img"; 
    int fd = create_disk(filename,DISK_SIZE);

    if(fd == -1){
        return 1;
    }

    cout<<"Virtual Disk Created."<<endl;
    cout<<"Number of blocks = "<< DISK_SIZE / BLOCK_SIZE << endl;
    close(fd);
    int num_writes = 100;
    verify_persistence(filename,num_writes);
}

