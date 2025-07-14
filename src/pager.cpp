#include "pager.h"

Pager* pager_open(const std::string& filename) {
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        std::cerr << "Unable to open file '" << filename << "'" << std::endl;
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);
    Pager* pager = new Pager();
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages = (file_length / PAGE_SIZE);

    if (file_length % PAGE_SIZE != 0) {
        std::cerr << "Db file is not a whole number of pages. Corrupt file." << std::endl;
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = nullptr;
    }

    return pager;
}

void* get_page(Pager* pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        std::cerr << "Tried to fetch page number out of bounds. " << page_num << " >= " << TABLE_MAX_PAGES << std::endl;
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == nullptr) {
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages_on_disk = pager->file_length / PAGE_SIZE;

        if (pager->file_length % PAGE_SIZE) {
            num_pages_on_disk += 1;
        }

        if (page_num < num_pages_on_disk) {
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1) {
                std::cerr << "Error reading file: " << strerror(errno) << std::endl;
                exit(EXIT_FAILURE);
            }
        } else {
            // This is a new page. Initialize it to all zeros.
            memset(page, 0, PAGE_SIZE);
        }

        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num + 1;
        }
    }
    return pager->pages[page_num];
}

void pager_flush(Pager* pager, uint32_t page_num) {
    if (pager->pages[page_num] == nullptr) {
        std::cerr << "Tried to flush null page." << std::endl;
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1) {
        std::cerr << "Error seeking: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);

    if (bytes_written == -1) {
        std::cerr << "Error writing to file: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

uint32_t get_unused_page_num(Pager* pager) {
    return pager->num_pages;
}
