#ifndef page_HEADER
#define page_HEADER

#include <inttypes.h>
#include <cstddef>

class Page {
    private :
        uint8_t * data;
        size_t size;
        Page * parent;
        
        void check_offset (size_t offset, size_t bytes);
        
    public :
        Page (size_t size);
        Page (size_t size, uint8_t * data);
        
        Page * destroy    ();
        Page * make_child ();
        
        void set_parent (Page * parent);
        
        void resize (size_t new_size);

        // copies data into this page
        // this does not affect the size of the page, and will throw an error
        // if size > this->size
        void s_data (const uint8_t * data, size_t size);

        size_t    g_size  ();
        uint8_t * g_data  (size_t offset);
        
        uint8_t   g_byte  (size_t offset);
        uint16_t  g_word  (size_t offset);
        uint32_t  g_dword (size_t offset);
        uint64_t  g_qword (size_t offset);
        
        void s_byte  (size_t offset, uint8_t  value);
        void s_word  (size_t offset, uint16_t value);
        void s_dword (size_t offset, uint32_t value);
        void s_qword (size_t offset, uint64_t value);
};

#endif
