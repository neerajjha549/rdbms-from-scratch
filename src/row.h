#ifndef ROW_H
#define ROW_H

#include <cstdint>
#include <cstring>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

struct Row {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
};

const uint32_t ID_SIZE = sizeof(uint32_t);
const uint32_t USERNAME_SIZE = sizeof(char) * (COLUMN_USERNAME_SIZE + 1);
const uint32_t EMAIL_SIZE = sizeof(char) * (COLUMN_EMAIL_SIZE + 1);
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

inline void serialize_row(const Row* source, void* destination) {
    memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);
    strncpy((char*)destination + USERNAME_OFFSET, source->username, COLUMN_USERNAME_SIZE);
    strncpy((char*)destination + EMAIL_OFFSET, source->email, COLUMN_EMAIL_SIZE);
}

inline void deserialize_row(const void* source, Row* destination) {
    memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), (char*)source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), (char*)source + EMAIL_OFFSET, EMAIL_SIZE);
    destination->username[COLUMN_USERNAME_SIZE] = '\0';
    destination->email[COLUMN_EMAIL_SIZE] = '\0';
}

#endif // ROW_H
