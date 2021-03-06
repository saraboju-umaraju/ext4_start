#include "ext4.h"

unsigned long long file_size = 0;
unsigned long long cur_size = 0;


void hexdump(char *buf, int size)
{
	int index = 0 ;
#if 0
	printf("entered %d\n", size);
	printf("%u %u|n", file_size, cur_size);
	return ;
#endif
	for (; size ; size--)
		printf("%c",*buf++);
}

void print_content(int f, int length, unsigned long long off)
{
	unsigned long long _off = lseek(f, 0, SEEK_CUR);

	lseek(f, off*4096L, SEEK_SET);

	char buffer[4096] = {} ;
	
	int j = 0 ;

	int length_1 = 0; 

	for(; j<length; j++) {
		if ( (file_size-cur_size) > 4096)
			length_1 = 4096;
		else
			length_1 = file_size % 4096 ;

		read(f, buffer, length_1);
		hexdump(buffer, length_1);
		cur_size+=length_1;
	}

	lseek(f, _off, SEEK_SET);
	
}

void parse_ex_tree(int f, unsigned long long off) 
{
	struct ext4_extent_header eh ;
	struct ext4_extent_idx    ehid;
	struct ext4_extent        ex ;
	struct ext4_extent        *exp ;

	//memcpy(&eh, addr, sizeof (eh) );		
	if (off)
		lseek(f, off*4096L, SEEK_SET);

	read(f, &eh, sizeof (eh));

#if 0	
	printf ("magic %x\n", eh.eh_magic);
	printf ("entries %x\n", eh.eh_entries);
	printf ("depth %x\n", eh.eh_depth);
	printf ("max entries %x\n", eh.eh_max);
#endif
	if (eh.eh_depth == 0 ) {
		int j = 0;
		for (; j<eh.eh_entries; j++) {
#if 0
			memcpy(&ex, 
					(char*)addr+(sizeof(eh)+(j*sizeof(ex)))  , 
					sizeof(ex));
#endif
			read(f, &ex, sizeof(ex));
#if 0
			printf ("block    %d\n",     ex.ee_block);
			printf ("len      %d\n", 	 ex.ee_len);
			printf ("start %d\n", 	 ex.ee_start_lo | (ex.ee_start_hi << 32));
			if ( ex.ee_block != ex.ee_block+ex.ee_len-1 )
				printf ("(%d-%d):%d-%d, ",ex.ee_block, ex.ee_block+ex.ee_len-1, 
						ex.ee_start_lo | (ex.ee_start_hi << 32),
						(ex.ee_start_lo | (ex.ee_start_hi << 32)) + ex.ee_len-1);
			else
				printf ("(%d):%d, ",ex.ee_block, 
						ex.ee_start_lo | (ex.ee_start_hi << 32));
#endif
			
			print_content(f, ex.ee_len, ex.ee_start_lo | (ex.ee_start_hi << 32));
		}
	}else {
		int j = 0;
		for (; j<eh.eh_entries; j++) {
#if 0
			memcpy(&ehid, 
					(char*)addr+(sizeof(eh)+(j*sizeof(ehid)))  , 
					sizeof(ehid));
#endif
			read(f, &ehid, sizeof(ehid));
#if 0
			printf ("block      %d\n",     ehid.ei_block);
			printf ("leaf      %d\n", 	   ehid.ei_leaf_hi);
			printf ("unused     %d\n",     ehid.ei_unused);
#endif
			//printf ("(ETB0):%d, ",      ehid.ei_leaf_lo| (ehid.ei_leaf_hi <<32));
			parse_ex_tree( f, ehid.ei_leaf_lo| (ehid.ei_leaf_hi <<32) );
		}

	}
}
int main (int argc, char *argv[])
{
	int f;
	int index = 0;
	int itable ;
	char content[4096] = {} ;
	struct ext4_inode ext4_inode;
	struct ext4_group_desc gd ;

	struct ext4_extent_header eh ;
	struct ext4_extent_idx    ehid;
	struct ext4_extent        ex ;
	struct ext4_extent        *exp ;

	int f_size = 0;

	if (argc < 3) {
		printf ("Throw me an Disk name and Inode boss !!\n");
		return 1;
	}

	f = open(DISK_NAME, O_RDONLY);

	if ( f == -1 ) {
		printf ("run with root permissions\n");
		return -1;
	}

	int inode = atoi(argv[2]) -1;

	int group = inode/8192 ;

	int index_1 = inode % 8192;

	int offset = index_1 * 256;

	int gd_count = get_gd_count(f);

	//printf ("gd size %d\n", gd_count);

	lseek(f, 1024+1024, SEEK_CUR);

	// leave out these x86 boot sector and padding && //readout superblock 

	lseek(f, 32 * (sizeof(struct ext4_group_desc)), SEEK_CUR);

	int size_of_gd = get_desc_size(f);

	for (; index < gd_count; index++){

		read(f, &gd, size_of_gd);

		if  (gd.bg_checksum && index == group) {

			//print_gd_info(index, &gd) ;

			itable = (((ext4_64_t)(gd.bg_inode_table_hi) << 32) | gd.bg_inode_table_lo);

			memset(&gd, 0, sizeof (gd));
		}
	}

	int constant = 1 ;

	lseek(f, 0, SEEK_SET);

	lseek(f, itable*4096L, SEEK_CUR);

	lseek(f, offset, SEEK_CUR);

	memset(&ext4_inode, 0, sizeof (struct ext4_inode));

#define offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)

	unsigned long long _off = read(f, 0, SEEK_CUR);

	read(f, &ext4_inode, sizeof (struct ext4_inode));

	if ( ! S_ISLNK(ext4_inode.i_mode) && ! S_ISREG(ext4_inode.i_mode)) {
		printf ("Not a Regular file\n");
		return -1 ;
	}

	if (S_ISLNK(ext4_inode.i_mode)) {
		char *uma = ext4_inode.i_block ;
		printf ("%s\n", uma);
	}else {

#if 1
		int loop_index = 0 ;
#if 0
		printf ("Iblock %u\n", ext4_inode.i_block);

		for (index=0; index < EXT4_N_BLOCKS ; index++) {
			printf (" block pointer. %d = %d\n", index, ext4_inode.i_block[index]);
		}
#endif
		lseek(f, -sizeof (struct ext4_inode), SEEK_CUR);
		lseek(f, offsetof(struct ext4_inode, i_block), SEEK_CUR);

		file_size = ((ext4_inode.i_size_high << 32) | ext4_inode.i_size_lo);
		parse_ex_tree(f, 0);
		//puts("");

#endif
	}

	return ;
	printf ("pid %d \n", getpid());

	while(1);
}

