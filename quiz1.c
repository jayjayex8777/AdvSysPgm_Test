#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>

    
static int movies[][2] = {
        {1997, 4},
        {1980, 5},
        {1983, 6},
        {1999, 1},
        {2002, 2},
        {2005, 3},
        {2015, 7},
        {2017, 8}
};

static char title_temp[][100] = {
        "Star Wars",
        "The Empire Strikes Back",
        "Return of the Jedi",
        "Star Wars: Episode I - The Phantom Menace",
        "Star Wars: Episode II - Attach of the Clones",
        "Star Wars: Episode III - Revenge of the Sith",
        "Star Wars: The Force Awakens",
        "Star Wars: The Last Jedi"
};

struct starwars_movie {
    int year;
    int episode;
    char title[100];
    struct list_head list;
};

static LIST_HEAD(movie_list);

#define TOTAL_NUM_OF_EPISODES 8  

int kernel_init(void)
{
    printk(KERN_INFO "Initializing the Starwars module\n");

    int i;

    for (i = 0; i < TOTAL_NUM_OF_EPISODES; i++) {
        struct starwars_movie *new_movie;
                
        new_movie = kmalloc(sizeof(*new_movie), GFP_KERNEL);
        new_movie->year = movies[i][0];
        new_movie->episode = movies[i][1];
        strcpy(new_movie->title,title_temp[i]);
        
        list_add_tail(&new_movie->list, &movie_list);
    }

    struct starwars_movie *movie;
    list_for_each_entry(movie, &movie_list, list) {
        printk(KERN_INFO "Opening year : %d, Title : %s, Episode number : %d\n", movie->year,movie->title,movie->episode);
    }

    return 0;
}

void kernel_exit(void)
{
   struct starwars_movie *movie, *tmp;
    
    list_for_each_entry_safe(movie, tmp, &movie_list, list) {
        printk(KERN_INFO "Delete episode : %d\n", movie->episode);
        list_del(&movie->list);
        kfree(movie);
    }

}

module_init(kernel_init);
module_exit(kernel_exit);

MODULE_LICENSE("GPL");  
