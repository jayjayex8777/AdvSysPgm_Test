#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>

struct starwars_movie {
    int episode;
    int year;
    struct list_head list;
};

static LIST_HEAD(movie_list);
struct list_head

int kernel_init(void)
{
    printk(KERN_INFO "Initializing the Starwars module\n");

    // 스타워즈 에피소드 정보
    int movies[][2] = {
        {1, 1999},
        {2, 2002},
        {3, 2005},
        {4, 1977},
        {5, 1980},
        {6, 1983},
    };
    int i;

    for (i = 0; i < sizeof(movies) / sizeof(movies[0]); i++) {
        struct starwars_movie *new_movie;
        
        // 메모리 할당
        new_movie = kmalloc(sizeof(*new_movie), GFP_KERNEL);
        new_movie->episode = movies[i][0];
        new_movie->year = movies[i][1];
        
        // 리스트에 추가
        list_add_tail(&new_movie->list, &movie_list);
    }

    struct starwars_movie *movie;
    list_for_each_entry(movie, &movie_list, list) {
        printk(KERN_INFO "Star Wars Episode %d - Year: %d\n", movie->episode, movie->year);
    }

    return 0;
}

void kernel_exit(void)
{
   struct starwars_movie *movie, *tmp;
    list_for_each_entry_safe(movie, tmp, &movie_list, list) {
        printk(KERN_INFO "Removing Episode %d - Year: %d\n", movie->episode, movie->year);
        list_del(&movie->list);
        kfree(movie);
    }

    printk(KERN_INFO "Removing the Starwars module\n");
}

module_init(kernel_init);
module_exit(kernel_exit);

MODULE_LICENSE("GPL");  
