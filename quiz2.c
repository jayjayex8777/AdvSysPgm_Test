#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>

struct starwars_episode {
    int episode_number;
    int year;
    struct hlist_node hash_node;
};

#define MAX_EPISODES 8
DEFINE_HASHTABLE(episodes_table, 2); // 버킷 크기를 4로 설정

int kernel_init(void)
{
    printk(KERN_INFO "Initializing the Starwars module\n");

    // 스타워즈 에피소드 정보
    int episodes[MAX_EPISODES][2] = {
        {1, 1999},
        {2, 2002},
        {3, 2005},
        {4, 1977},
        {5, 1980},
        {6, 1983},
        {7, 2015},
        {8, 2019},
    };

    int i;
    struct starwars_episode *episode;

    // 해시 테이블 초기화
    hash_init(episodes_table);

    // 스타워즈 에피소드 정보를 해시 테이블에 추가
    for (i = 0; i < MAX_EPISODES; i++) {
        episode = kmalloc(sizeof(*episode), GFP_KERNEL);
        if (!episode)
            return -ENOMEM;

        episode->episode_number = episodes[i][0];
        episode->year = episodes[i][1];
        
        // 에피소드 숫자를 해시 값으로 사용하여 해시 테이블에 추가
        hash_add(episodes_table, &episode->hash_node, episode->episode_number);
    }

    // 해시 테이블을 순회하며 버킷 별로 에피소드 정보 출력
    struct hlist_node *tmp;
    struct starwars_episode *e;
    int bkt;
    for (bkt = 0; bkt < 3; bkt++) { // 버킷 번호를 0부터 1까지
        printk(KERN_INFO "Bucket Number: %d\n", bkt);
        hash_for_each_possible(episodes_table, e, hash_node, bkt) {
            printk(KERN_INFO "Star Wars Episode %d - Year: %d\n", e->episode_number, e->year);
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
