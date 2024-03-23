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

DEFINE_HASHTABLE(episodes_table, 2); //bucket size set to 4

int kernel_init(void)
{
	int i;
	struct starwars_episode *episode;

	hash_init(episodes_table);

	for (i = 0; i < MAX_EPISODES; i++) {
		
		episode = kmalloc(sizeof(*episode), GFP_KERNEL);

		if (!episode)
			return -ENOMEM;

		episode->year = movies[i][0];
		episode->episode_number = movies[i][1];       
		strcpy(episode->title,title_temp[i]);
	
		hash_add(episodes_table, &episode->hash_node, episode->episode_number);
	}

	struct hlist_node *tmp;
	struct starwars_episode *e;

	int bkt = 0;

	hash_for_each(episodes_table, bkt, e, hash_node) {
		pr_info("Opening year : %d, Title : %s, Episode number : %d, bkt : %d\n"\
		,e->year, e->title, e->episode_number,bkt);
	} 
	
	pr_info("\n\n");
	hash_for_each_possible(episodes_table, e, hash_node, 1){

		if (e->episode_number == 1 || e->episode_number == 8) {
			pr_info("Opening year : %d, Title : %s, Episode number : %d, bkt : %d\n"\
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

	hash_for_each_safe(episodes_table, bkt, tmp, e, hash_node) {
		hash_del(&e->hash_node);
		kfree(e);
	}

	pr_info("Removing the Starwars module\n");
}

module_init(kernel_init);
module_exit(kernel_exit);

MODULE_LICENSE("GPL");  



