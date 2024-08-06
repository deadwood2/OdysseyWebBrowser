#ifndef OWB_BOOKMARKGROUPCLASS_H
#define OWB_BOOKMARKGROUPCLASS_H

#define NODEFLAG_GROUP     1
#define NODEFLAG_LINK      2
#define NODEFLAG_BAR       4
#define NODEFLAG_INMENU    8
#define NODEFLAG_QUICKLINK 16
#define NODEFLAG_NOTATTACHED 32

struct treedata
{
	ULONG  flags;   // flags for show in menu, bar & groups
	STRPTR title;
	STRPTR alias;
	STRPTR address;
	STRPTR buffer1;
	STRPTR buffer2;
	ULONG  ql_order;
	APTR   icon;
	APTR   iconimg;
	ULONG  showalias;
	APTR   tree;
	APTR   treenode;
};

enum
{
	POPMENU_OPEN_CURRENT = 1,
	POPMENU_OPEN_NEWTAB,
	POPMENU_OPEN_NEWWIN,
	POPMENU_OPEN,
	POPMENU_OPEN_ALL,
	POPMENU_CLOSE,
	POPMENU_CLOSE_ALL,
	POPMENU_REMOVE
};


#endif

