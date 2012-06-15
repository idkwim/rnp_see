#include "memory.h"

#include <stdexcept>
#include <sstream>

uint64_t Memory :: g_page_address (uint64_t address)
{
    std::map <uint64_t, Page *> :: iterator it;
    it = pages.upper_bound(address);
    if (it != pages.begin()) {
        it--;
        if (it->second->g_size() + it->first < address)
            return address;
    }

    std::stringstream ss;
    ss << "memory addressed dereferenced but not paged: 0x" << std::hex << address;
    throw std::runtime_error(ss.str());
    
}

uint8_t Memory :: g_byte (uint64_t address)
{
    uint64_t page_address = g_page_address(address);
    return this->pages[page_address]->g_byte(address - page_address);
}

uint16_t Memory :: g_word (uint64_t address)
{
    uint64_t page_address = g_page_address(address);
    return this->pages[page_address]->g_word(address - page_address);
}

uint32_t Memory :: g_dword (uint64_t address)
{
    uint64_t page_address = g_page_address(address);
    return this->pages[page_address]->g_dword(address - page_address);
}

uint64_t Memory :: g_qword (uint64_t address)
{
    uint64_t page_address = g_page_address(address);
    return this->pages[page_address]->g_qword(address - page_address);
}

void Memory :: s_byte (uint64_t address, uint8_t value)
{
    uint64_t page_address = g_page_address(address);
    this->pages[page_address]->s_byte(address - page_address, value);
}

void Memory :: s_word (uint64_t address, uint16_t value)
{
    uint64_t page_address = g_page_address(address);
    this->pages[page_address]->s_word(address - page_address, value);
}

void Memory :: s_dword (uint64_t address, uint32_t value)
{
    uint64_t page_address = g_page_address(address);
    this->pages[page_address]->s_dword(address - page_address, value);
}

void Memory :: s_qword (uint64_t address, uint64_t value)
{
    uint64_t page_address = g_page_address(address);
    this->pages[page_address]->s_qword(address - page_address, value);
}

std::string Memory :: memmap ()
{
    std::stringstream ss;
    std::map <uint64_t, Page *> :: iterator it;

    for (it = pages.begin(); it != pages.end(); it++) {
        ss << std::hex << it->first
           << "\t" << "size=" << std::hex << it->second-> g_size()
           << std::endl;
    }

    return ss.str();
}