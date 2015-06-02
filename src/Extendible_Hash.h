#ifndef EH_H
#define EH_H

/*---------------------宏定义-------------------*/
#define PAGE_NUMBER 128								//页面个数
#define PAGE_SIZE 8192								//页面大小
#define INT sizeof(int)								//整型的大小
#define CHAR sizeof(char)							//字符型的大小
#define MOST 1										//表示启用高位扩展模式
#define LEAST 0										//表示启用低位扩展模式

/*---------------------数据结构定义-------------------*/

/*----------内存页面结构----------*/
typedef struct{
	void* Pointer;			//指向内存空间的指针
	int Curr_Size;			//页面当前可用大小
	int BucketID;			//对应桶的ID（表示该号桶占用此页面）
	bool Used;				//页面是否已被使用
	bool Refered;			//页面是否被访问
	bool Locked;			//页面是否被锁定（用于防止被时钟页面置换算法置换）
}Page;
/*----------哈希桶结构----------*/
typedef struct{
	int BucketID;			//桶号
	int PageID;				//对应页面ID（表示此桶所占用页面的号码）
	char* Head;				//指针用于指向桶的开端
	int* Tail;				//指针用于指向桶的末尾
	int Position;			//表示在内存中驻留的位置
}Bucket;

/*----------元组结构----------*/
typedef struct{
	char Info[200];			//元组所有的信息
	int Lenth;				//元组的长度
}Tuple;

Page Page_Set[PAGE_NUMBER];							//内存中可用页面的集合
Bucket Bucket_Set[PAGE_NUMBER-2];					//驻留在内存中桶的集合
													//页面通过页面号来进行定位，而桶则通过桶位置来定位
int Bucket_Number;									//桶的数量
int* Index;											//索引指针
int Index_Size;										//索引大小（按页面大小计算，1表示小于一个页面大小）
int Index_Offset;									//目前在内存中的索引相对整个索引文件的偏移值
int Clock;											//时钟置换算法的指针
int Global_Depth;									//全局深度
bool Exten_Model;									//哈希函数扩展方式
char* Read_Buffer;									//读数据文件的指针（后来发现其实不需要做成全局变量，后面用不上）
int IO;												//记录I/O的次数
/*----------以下字符数组均作为启动函数路径使用----------*/
char INDEX[100];
char BUCKET[100];
char IN[100];
char OUT[100];
char TBL[100];
char TEMP[100];

/*---------------------函数定义-------------------*/
/*		此处简要概括，在函数实现部分有详细说明		*/

/*----------主要函数----------*/
void Initialize();									//全局初始化函数
void Read_Tuple();									//向tbl文件中读取数据
void Make_Index(Tuple);								//根据索引表找到元组合适的桶
void Put_Into_Bucket(Tuple,int);					//将元组放入桶前的所有条件判断
void Put_Into(Tuple,int);							//将元组放入指定桶内
int Get_Page(int&);									//申请一个页面
void Reset(int);									//重置一个页面
void Search();										//查询
void Match_And_Output(int,int);						//输出匹配的结果
void Quick_Sort(int*,Tuple*,int);					//快速排序
void Del_All_Page();								//释放所有页面的空间

/*----------辅助函数----------*/
void InitPage(int);									//初始化一个页面
void Init_Bucket_Set(int);							//初始化指定位置的桶
void Bucket_Load(int,int,int);						//加载指定桶到指定页面
int Get_IndexID_From_Bucket(int);					//从根据桶中的元组获得对应的索引值
int Get_Size_From_Bucket(int);						//获得桶的剩余空间
int Get_BucketID(Tuple);							//获得元组应该放入的桶号
int Get_Search_Key(Tuple);							//从元组中提取搜索键						
int Get_Part_Key(Tuple);							//从元组中提取排序键
void Quick_Sort_Recursion(int*,Tuple*,int,int);		//快速排序的递归函数

/*-----高位扩展使用-----*/
void Exten_Index_Itself_Most();						//哈希索引表扩展，在页面内
void Exten_Index_Page_Most();						//哈希索引表扩展，页面之间
void Bucket_Split_Most(int);						//桶分裂
int Get_IndexID_Most(int,int);						//获得元组的对应索引值
int Exten_Decimal(int,int);							//十进制形式下的扩展，用于辅助桶分裂

/*-----低位扩展使用-----*/							//以下函数功能同高位扩展
void Exten_Index_Itself_Least();
void Exten_Index_Page_Least();
void Bucket_Split_Least(int);
int Get_IndexID_Least(int,int);

#endif
