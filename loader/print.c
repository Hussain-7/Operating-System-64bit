#include "stdint.h"
#include "stdarg.h"
#include "print.h"
#include "lib.h"

static struct ScreenBuffer screen_buffer = {(char*)(0xb8000), 0, 0};


static int udecimal_to_string(char *buffer, int position, uint64_t digits)
{
    char digits_map[10] = "0123456789";
    char digits_buffer[25];
    int size = 0;

    do {
        digits_buffer[size++] = digits_map[digits % 10];
        digits /= 10;
    } while (digits != 0);

    for (int i = size-1; i >= 0; i--) {
        buffer[position++] = digits_buffer[i];
    }

    return size;
}

static int decimal_to_string(char *buffer, int position, int64_t digits)
{
    int size = 0;

    if (digits < 0) {
        digits = -digits;
        buffer[position++] = '-';
        size = 1;
    }

    size += udecimal_to_string(buffer, position, (uint64_t)digits);
    return size;
}

static int hex_to_string(char *buffer, int position, uint64_t digits)
{
    char digits_buffer[25];
    char digits_map[16] = "0123456789ABCDEF";
    int size = 0;

    do {
        digits_buffer[size++] = digits_map[digits % 16];
        digits /= 16;
    } while (digits != 0);

    for (int i = size-1; i >= 0; i--) {
        buffer[position++] = digits_buffer[i];
    }

    buffer[position++] = 'H';

    return size+1;
}

static int read_string(char *buffer, int position, const char *string)
{
    int index = 0;
    // since we want to start writting from right after %s or d,u etc start writting from position index 
    for (index = 0; string[index] != '\0'; index++) {
        buffer[position++] = string[index];
    }

    return index;
}
void clear_screen(){
    struct ScreenBuffer *sb=&screen_buffer;
    for(int i=0;i<24;i++)
        memset(sb->buffer+LINE_SIZE*i,0,LINE_SIZE);

    sb->column=0;
    sb->row=0;
}

void write_screen(const char *buffer, int size,  char color)
{
    struct ScreenBuffer *sb=&screen_buffer;
    int column = sb->column;
    int row = sb->row;

    for (int i = 0; i < size; i++) {       
        if (row >= 25) {
            // we copy the characters which are in the second line through the last line to the first to 24th line
            memcpy(sb->buffer,sb->buffer+LINE_SIZE,LINE_SIZE*24);
            // then set last line to empty
            memset(sb->buffer+LINE_SIZE*24,0,LINE_SIZE);
            row--;
        }
        
        if (buffer[i] == '\n') {
            column = 0;
            row++;
        } 
        else if (buffer[i] == '\b') { 
            if (column == 0 && row == 0) {
                continue;
            }
            
            if (column == 0) {
                row--;
                column = 80;
            }

            column -= 1;
            sb->buffer[column*2+row*LINE_SIZE] = 0;
            sb->buffer[column*2+row*LINE_SIZE+1] = 0;  
        }
        else {
            sb->buffer[column*2+row*LINE_SIZE] = buffer[i];
            sb->buffer[column*2+row*LINE_SIZE+1] = color;

            column++;

            if (column >= 80) {
                column=0;
                row++;
            }
        }
    }

    sb->column = column;
    sb->row = row;
}

int printk(const char *format, ...)
{
    char buffer[1024];
    int buffer_size = 0;
    int64_t integer = 0;
    char *string = 0;
    va_list args;

    va_start(args,format);

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            buffer[buffer_size++] = format[i];
        }
        else {
            switch (format[++i]) {
                case 'x':
                    integer = va_arg(args, int64_t);
                    buffer_size += hex_to_string(buffer, buffer_size, (uint64_t)integer);
                    break;

                case 'u':
                    integer = va_arg(args, int64_t);
                    buffer_size += udecimal_to_string(buffer, buffer_size, (uint64_t)integer);
                    break;

                case 'd':
                    integer = va_arg(args, int64_t);
                    buffer_size += decimal_to_string(buffer, buffer_size, integer);
                    break;

                case 's':
                    // we use va_arg macro to retrieve pointer. first param is arg variable second it  type of variable we want to return
                    string = va_arg(args, char*);
                    buffer_size += read_string(buffer, buffer_size, string);
                    break;

                default:
                    buffer[buffer_size++] = '%';
                    i--;
            }
        }     
    }

     if(!memcmp("shell# ",buffer,7))
    {
        buffer_size = writeu(buffer, buffer_size,0xe);
    }
    else
    {
        buffer_size = writeu(buffer, buffer_size,0x7);
    }
    va_end(args);
    //return total number of characters being printed
    return buffer_size;
}
