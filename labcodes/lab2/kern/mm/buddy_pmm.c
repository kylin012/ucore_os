#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_pmm.h>

#define LEFT_LEAF(index) ((index)*2 + 1)
#define RIGHT_LEAF(index) ((index)*2 + 2)
#define PARENT(index) (((index) + 1) / 2 - 1)

#define IS_POWER_OF_2(x) (!((x) & ((x)-1)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int ROUND_UP_LOG(int size){
    int n = 0, tmp = size;
    while(tmp >>= 1){
        n++;
    }
    tmp = (size >> n) << n;
    n += ((size - tmp) !=0);
    return n;
}

int* buddy_manager;

struct Page* page_base;

int total_pages, free_pages, manager_size;

static void
buddy_init(void) {
    
}

static void
buddy_init_memmap(struct Page *base, size_t n){
    page_base = base;
    free_pages = n;
    manager_size = 2 * (1<<ROUND_UP_LOG(n));
    buddy_manager = (int*)malloc(sizeof(int) * manager_size);
    struct Page* p;
    for(p = base; p != base + n; p++){
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
        SetPageProperty(p);
    }
    // attention that it is valid from 1
    int i = 1;
    int node_size = manager_size / 2;
    for(; i < manager_size; i++){
        buddy_manager[i] = node_size;
        if(IS_POWER_OF_2(i+1)){
            node_size /= 2;
        }
    }
    base->property = n;
    SetPageProperty(base);
}

static struct Page*
buddy_alloc_pages(size_t n) {

}

static void
buddy_free_pages(struct Page* base, size_t n) {

}

static size_t
buddy_nr_free_pages(void) {

}

static void
basic_check(void) {

}

static void
buddy_check(void) {

}

const struct pmm_manager default_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};