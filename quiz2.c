#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/hashtable.h>

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


struct starwars_episode {
    int year;    
    int episode_number;
    char title[100];    
    struct hlist_node hash_node;
};

#define MAX_EPISODES 8
DEFINE_HASHTABLE(episodes_table, 2); // 버킷 크기를 4로 설정

int kernel_init(void)
{
    printk(KERN_INFO "Initializing the Starwars module\n");

    int i;
    struct starwars_episode *episode;

    // 해시 테이블 초기화
    hash_init(episodes_table);

    // 스타워즈 에피소드 정보를 해시 테이블에 추가
    for (i = 0; i < MAX_EPISODES; i++) {
        episode = kmalloc(sizeof(*episode), GFP_KERNEL);
        if (!episode)
            return -ENOMEM;
 
        episode->year = movies[i][0];
        episode->episode_number = movies[i][1];       
        strcpy(episode->title,title_temp[i]);
        
        // 에피소드 숫자를 해시 값으로 사용하여 해시 테이블에 추가
        hash_add(episodes_table, &episode->hash_node, episode->episode_number);
    }

    // 해시 테이블을 순회하며 버킷 별로 에피소드 정보 출력
    struct hlist_node *tmp;
    struct starwars_episode *e;
    int bkt;
    for (bkt = 0; bkt < 4; bkt++) { // 버킷 번호를 0부터 1까지
        printk(KERN_INFO "Bucket Number: %d\n", bkt);
        hash_for_each_possible(episodes_table, e, hash_node, bkt) {
            printk(KERN_INFO "Opening year : %d, Title : %s, Episode number : %d, bkt : %d\n"\
                ,e->year, e->title, e->episode_number,bkt);
        }
    }

    return 0;
}

void kernel_exit(void)
{
   struct starwars_episode *e;
    struct hlist_node *tmp;
    int bkt;

    // 해시 테이블을 순회하며 각 버킷의 모든 요소를 제거하고 메모리 해제
    hash_for_each_safe(episodes_table, bkt, tmp, e, hash_node) {
        hash_del(&e->hash_node);
        kfree(e);
    }

    printk(KERN_INFO "Removing the Starwars module\n");

}

module_init(kernel_init);
module_exit(kernel_exit);

MODULE_LICENSE("GPL");  
