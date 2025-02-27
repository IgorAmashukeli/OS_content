#include <elf.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>

#include "backtrace.h"


/**open file with error checking**/
#define FOPEN(filename, flags, file) \
do {\
    file = fopen(filename, flags);\
    if (file == NULL) {\
        perror("fopen");\
        exit(EXIT_FAILURE);\
    }\
} while(false)\



#define FCLOSE(file)\
do {\
    int code = fclose(file);\
    if (code == EOF) {\
        perror("fclose");\
        exit(EXIT_FAILURE);\
    }\
} while(false)\



/**mmap with cast and with error checking**/
#define MMAP(type, res, address, length, protect, flags, filedes, offset)\
do {\
    res = (type) mmap((void*)address, (size_t)length, protect, flags, filedes, (off_t)offset);\
    if (res == MAP_FAILED) {\
        perror("mmap");\
        exit(EXIT_FAILURE);\
    }\
} while (false)\



/**munmap with error checking**/
#define MUNMAP(res, length)\
do {\
    int err = munmap(res, length);\
    if (err == -1) {\
        perror("munmap");\
        exit(EXIT_FAILURE);\
    }\
} while(false)\



/**read to data items_count elements with size "size" from file "file" with error checking"**/
#define FREAD(data, size, items_count, file) \
do {\
    int code = fread(data, size, items_count, file);\
    if (code == 0) {\
        perror("fread");\
        exit(EXIT_FAILURE);\
    }\
} while(false)\




/**get section header index

0) if section header is not UNDEF or SH_XINDEX, we just cast ELF64_Half
to Elf64_Word

1) if section header is UNDEF => file has no section name section table
=> exit

2) if section header is SHN_XINDEX (0xffff), section header is bigger than
0xfff0, and real header contains in sh_link of the first section entry in section header
 table
=> therefore we mmap, get sh_link, it is ELF64_Word, munmap

**/
#define GET_SECTION_HEADER_INDEX(section_offset, elf_file,\
shstrtab_section_header_index, elf_header) \
do {\
    shstrtab_section_header_index = (Elf64_Word) elf_header.e_shstrndx;\
    if (shstrtab_section_header_index == SHN_UNDEF) {\
        perror("e_shstrndx");\
        exit(EXIT_FAILURE);\
    }\
    \
    \
    if (shstrtab_section_header_index == SHN_XINDEX) {\
        Elf64_Shdr sec_hdr;\
        fseek(elf_file, section_offset, SEEK_SET);\
        FREAD(&sec_hdr, sizeof(Elf64_Shdr), 1, elf_file);\
        shstrtab_section_header_index = sec_hdr.sh_link;\
    }\
} while(false)\


/**get elf header of elf_file**/
void get_elf_header(FILE* elf_file, Elf64_Ehdr* elf_header_pointer) {
    fseek(elf_file, 0, SEEK_SET);
    FREAD(elf_header_pointer, sizeof(Elf64_Ehdr), 1, elf_file);
}


/**

get:

1) section headers offset

2) section headers size

3) section headers number

**/
void get_section_headers_info(Elf64_Ehdr elf_header, Elf64_Off* section_header_offset_pointer,
 Elf64_Half* section_header_size_pointer, Elf64_Half* section_header_number_pointer) {

    // find offset for section headers in file
    *section_header_offset_pointer = elf_header.e_shoff;

    // find size of each section header in file
    *section_header_size_pointer = elf_header.e_shentsize;

    // find number of section headers in file
    *section_header_number_pointer = elf_header.e_shnum;
}




/**get index of shstrtab section header index**/
void get_shstrtab_section_header_index(Elf64_Off section_header_offset, FILE* elf_file,
 Elf64_Word* shstrtab_section_header_index_ptr, Elf64_Ehdr elf_header) {
    GET_SECTION_HEADER_INDEX(section_header_offset, elf_file,
     *shstrtab_section_header_index_ptr, elf_header);
}




/**get Elf64_Shdr section header for shstrtab**/
Elf64_Shdr* get_shstrtab_section_header(FILE* elf_file, Elf64_Ehdr* elf_header,
Elf64_Shdr* shstrtab_section_header,
Elf64_Off section_header_offset,
 Elf64_Half section_header_size, Elf64_Word shstrtab_section_header_index) {

    // first cast to char* to operate in bytes, then calculate, then case to type
    fseek(elf_file, section_header_offset + section_header_size * shstrtab_section_header_index, SEEK_SET);
    FREAD(shstrtab_section_header, sizeof(Elf64_Shdr), 1, elf_file);

    return shstrtab_section_header;
}





/**
get section for shstrtab/strtab
**/
void get_str_section(FILE* elf_file,
char* tab_section, uint64_t section_size,
Elf64_Off tab_section_offset) {

        fseek(elf_file, tab_section_offset, SEEK_SET);
        FREAD(tab_section, section_size, 1, elf_file);
}




/**get section for symtab**/
void get_symtab_section(FILE* elf_file,
Elf64_Sym* section, uint64_t section_size,
Elf64_Off tab_section_offset) {


        fseek(elf_file, tab_section_offset, SEEK_SET);
        FREAD(section, section_size, 1, elf_file);
}




/**get section header for section_name section**/
Elf64_Shdr* get_section_header(FILE* elf_file,
Elf64_Shdr* section_header,
char* shstrtab_section, Elf64_Off section_header_offset,
    Elf64_Half section_header_size,
    Elf64_Half section_header_number, char* section_name) {

        for (Elf64_Half i = 0; i < section_header_number; ++i) {

            // get current section header
            fseek(elf_file, section_header_offset + section_header_size * i, SEEK_SET); 
            FREAD(section_header, section_header_size, 1, elf_file);

            // get position in shstrtab section for name of the section
            uint32_t cur_section_name_pos = section_header->sh_name;

            // get current name of the section from shstrtab section
            char* cur_section_name = shstrtab_section + cur_section_name_pos;

            if (strcmp(cur_section_name, section_name) == 0) {
                return section_header;
            }


        }

        return NULL;
}



/**get section headers for strtab and symtab**/
#define get_strtab_symtab_section_headers(elf_file,\
shstrtab_section,\
    section_header_offset,\
    section_header_size,\
    section_header_number,\
    strtab_section_header,\
    symtab_section_header)\
do {\
    char* strtab_name = ".strtab";\
    char* sym_tab = ".symtab";\
    get_section_header(elf_file,\
    strtab_section_header, shstrtab_section,  section_header_offset,\
       section_header_size,  section_header_number, strtab_name);\
    get_section_header(elf_file,\
      symtab_section_header, shstrtab_section,  section_header_offset,\
       section_header_size,  section_header_number, sym_tab);\
    if (strtab_section_header == NULL || symtab_section_header == NULL) {\
        perror("find section");\
        exit(EXIT_FAILURE);\
    }\
} while(false)\







/**get name position of function in strtab**/
uint32_t get_name_position(void* addr, Elf64_Sym* sym_table,
 uint64_t symtab_section_entry_count, uint64_t symtab_section_size) {
    // name position in strtab section
    uint32_t name_pos = 0;

    // function address of function
    Elf64_Addr res_func_address = 0;

    // iterate through all symbols in symbol table
    for (uint64_t i = 0; i < symtab_section_entry_count; ++i) {
        Elf64_Sym symbol = sym_table[i];

        // if found global (strong or weak) symbol, which is a function
        if ((ELF64_ST_BIND(symbol.st_info) == STB_GLOBAL ||
         ELF64_ST_BIND(symbol.st_info) == STB_WEAK)  &&
         ELF64_ST_TYPE(symbol.st_info) == STT_FUNC) {

            // function address
            Elf64_Addr func_address = symbol.st_value;

            // if function address is closer to addr, but not bigger,
            // update function address and function name position in strtab
            if (func_address >= res_func_address && func_address <= (Elf64_Addr) addr) {

                // update function address
                res_func_address = func_address;

                // update function name position in strtab
                name_pos = symbol.st_name;
            }
        }
    }


    if (name_pos == 0) {
        perror("symbol has no name");
        exit(EXIT_FAILURE);
    }

    return name_pos;
}




/**find name of the function by this address**/
char* addr2name(void* addr)
{
    // 1) open executable file
    FILE* elf_file;
    FOPEN("/proc/self/exe", "r", elf_file);


    // 2) get elf header
    Elf64_Ehdr elf_header;
    get_elf_header(elf_file, &elf_header);

    // 3) get section headers info:
    //  a) offset to the start of section headers
    //  b) size of one section header entry in bytes
    //  c) number of section header entries
    Elf64_Off section_header_offset;
    Elf64_Half section_header_size;
    Elf64_Half section_header_number;
    get_section_headers_info(elf_header,
     &section_header_offset,
      &section_header_size,
       &section_header_number);

    // 4) get index of the shstrtab section header
    Elf64_Word shstrtab_section_index;
    get_shstrtab_section_header_index(section_header_offset, elf_file,
     &shstrtab_section_index, elf_header);

    // 5) get shstrtab section header itself
    Elf64_Shdr shstrtab_section_header;
    get_shstrtab_section_header(elf_file, &elf_header,
     &shstrtab_section_header,
     section_header_offset,
 section_header_size, shstrtab_section_index);
    // 6) get shstrtab section offset and size
    Elf64_Off shstrtab_section_offset = shstrtab_section_header.sh_offset;
    uint64_t shstrtab_size = shstrtab_section_header.sh_size;


    // 7) get shstrtab section itself
    char shstrtab_section[shstrtab_size];
    get_str_section(elf_file, shstrtab_section, shstrtab_size, shstrtab_section_offset);

    // 8) get strtab and symtab section headers
    Elf64_Shdr strtab_section_header;
    Elf64_Shdr symtab_section_header;
    get_strtab_symtab_section_headers(elf_file,
     shstrtab_section, section_header_offset,
      section_header_size, section_header_number,
       &strtab_section_header, &symtab_section_header);

    // 9) get strtab section offset and size
    Elf64_Off strtab_section_offset = strtab_section_header.sh_offset;
    uint64_t strtab_size = strtab_section_header.sh_size;
    
    // 10)  get strtab section itself
    char strtab_section[strtab_size];
    get_str_section(elf_file, strtab_section, strtab_size, strtab_section_offset);


    // 11) get symtab section info:
    //   a) symtab section offset
    //   b) symtab section size
    //   c) symtab section entries count
    Elf64_Off symtab_section_offset = symtab_section_header.sh_offset;
    uint64_t symtab_section_size = symtab_section_header.sh_size;
    uint64_t symtab_section_entry_count = symtab_section_size / sizeof(Elf64_Sym);

    
    // 12) get symtab section itself
    Elf64_Sym symtab_section[symtab_section_entry_count];
    get_symtab_section(elf_file,
     symtab_section, symtab_section_size, symtab_section_offset);

    // there is no reason to calculate ASLR offset
    // (because code will be compiled with no-pie compilation keys)


    // 13) find the st_name (position in strtab) for function with addr address
    // and unmap symtab section

    uint32_t name_position = get_name_position(addr,
     symtab_section, symtab_section_entry_count, symtab_section_size);


    // 14) get function name
    char* function_name = strtab_section + name_position;

    // 15) close elf_file
    FCLOSE(elf_file);


    return function_name;
}







/**back trace and remember return addresses**/
int backtrace(void* backtrace[], int limit)
{
    // number of found addresses
    // backtrace[0] contains return address of backtrace
    int count = 0;


    // rbp
    void* rbp = NULL;
    // return address
    void* return_address = NULL;

    // find rbp of current frame
    asm volatile (
        // save rbp
            "movq %%rbp, %0\n\t"
        // %0 is rbp
            : "=r"(rbp)
            :
            :
        // Clobbers
            "memory",
            "cc"
    );





    // iterate limit times
    while (count != limit) {

        // iterate through stack frames
        // (save return address, move to previous rbp)
        asm volatile (
                // save return address of the function
                "movq 8(%%rdx), %%rax\n\t"

                // save old "rbp" variable in "rbp" variable
                "movq (%%rdx), %%rbx\n\t"

                // Output Operands:

                :
                // rbx is used for new value of "rbp"
                "=b"(rbp),

                // rax is used for return_address
                "=a"(return_address)


                // Input Operands
                :
                // rdx is used for old value of "rbp"
                "d"(rbp)

                // clobbers:
                :
                "memory",

                // some move can change flags
                "cc"
        );

        // found main => break

        // save return address in backtrace[count]:
        backtrace[count] = return_address;
        ++count;

        // if return address is not NULL and name is "main"
        if (strcmp(addr2name(return_address), "main") == 0) {
            break;
        }
    }


    return count;
}

void print_backtrace()
{
    int limit = 1024;
    void* backtrace_addresses[limit];
    int count = backtrace(backtrace_addresses, limit);
    for (size_t i = 0; i < count; ++i) {
        printf("0x%lx %s\n", (uintptr_t) backtrace_addresses[i],
         addr2name(backtrace_addresses[i]));
    }
}
