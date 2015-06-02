#ifndef EH_H
#define EH_H

/*---------------------�궨��-------------------*/
#define PAGE_NUMBER 128								//ҳ�����
#define PAGE_SIZE 8192								//ҳ���С
#define INT sizeof(int)								//���͵Ĵ�С
#define CHAR sizeof(char)							//�ַ��͵Ĵ�С
#define MOST 1										//��ʾ���ø�λ��չģʽ
#define LEAST 0										//��ʾ���õ�λ��չģʽ

/*---------------------���ݽṹ����-------------------*/

/*----------�ڴ�ҳ��ṹ----------*/
typedef struct{
	void* Pointer;			//ָ���ڴ�ռ��ָ��
	int Curr_Size;			//ҳ�浱ǰ���ô�С
	int BucketID;			//��ӦͰ��ID����ʾ�ú�Ͱռ�ô�ҳ�棩
	bool Used;				//ҳ���Ƿ��ѱ�ʹ��
	bool Refered;			//ҳ���Ƿ񱻷���
	bool Locked;			//ҳ���Ƿ����������ڷ�ֹ��ʱ��ҳ���û��㷨�û���
}Page;
/*----------��ϣͰ�ṹ----------*/
typedef struct{
	int BucketID;			//Ͱ��
	int PageID;				//��Ӧҳ��ID����ʾ��Ͱ��ռ��ҳ��ĺ��룩
	char* Head;				//ָ������ָ��Ͱ�Ŀ���
	int* Tail;				//ָ������ָ��Ͱ��ĩβ
	int Position;			//��ʾ���ڴ���פ����λ��
}Bucket;

/*----------Ԫ��ṹ----------*/
typedef struct{
	char Info[200];			//Ԫ�����е���Ϣ
	int Lenth;				//Ԫ��ĳ���
}Tuple;

Page Page_Set[PAGE_NUMBER];							//�ڴ��п���ҳ��ļ���
Bucket Bucket_Set[PAGE_NUMBER-2];					//פ�����ڴ���Ͱ�ļ���
													//ҳ��ͨ��ҳ��������ж�λ����Ͱ��ͨ��Ͱλ������λ
int Bucket_Number;									//Ͱ������
int* Index;											//����ָ��
int Index_Size;										//������С����ҳ���С���㣬1��ʾС��һ��ҳ���С��
int Index_Offset;									//Ŀǰ���ڴ��е�����������������ļ���ƫ��ֵ
int Clock;											//ʱ���û��㷨��ָ��
int Global_Depth;									//ȫ�����
bool Exten_Model;									//��ϣ������չ��ʽ
char* Read_Buffer;									//�������ļ���ָ�루����������ʵ����Ҫ����ȫ�ֱ����������ò��ϣ�
int IO;												//��¼I/O�Ĵ���
/*----------�����ַ��������Ϊ��������·��ʹ��----------*/
char INDEX[100];
char BUCKET[100];
char IN[100];
char OUT[100];
char TBL[100];
char TEMP[100];

/*---------------------��������-------------------*/
/*		�˴���Ҫ�������ں���ʵ�ֲ�������ϸ˵��		*/

/*----------��Ҫ����----------*/
void Initialize();									//ȫ�ֳ�ʼ������
void Read_Tuple();									//��tbl�ļ��ж�ȡ����
void Make_Index(Tuple);								//�����������ҵ�Ԫ����ʵ�Ͱ
void Put_Into_Bucket(Tuple,int);					//��Ԫ�����Ͱǰ�����������ж�
void Put_Into(Tuple,int);							//��Ԫ�����ָ��Ͱ��
int Get_Page(int&);									//����һ��ҳ��
void Reset(int);									//����һ��ҳ��
void Search();										//��ѯ
void Match_And_Output(int,int);						//���ƥ��Ľ��
void Quick_Sort(int*,Tuple*,int);					//��������
void Del_All_Page();								//�ͷ�����ҳ��Ŀռ�

/*----------��������----------*/
void InitPage(int);									//��ʼ��һ��ҳ��
void Init_Bucket_Set(int);							//��ʼ��ָ��λ�õ�Ͱ
void Bucket_Load(int,int,int);						//����ָ��Ͱ��ָ��ҳ��
int Get_IndexID_From_Bucket(int);					//�Ӹ���Ͱ�е�Ԫ���ö�Ӧ������ֵ
int Get_Size_From_Bucket(int);						//���Ͱ��ʣ��ռ�
int Get_BucketID(Tuple);							//���Ԫ��Ӧ�÷����Ͱ��
int Get_Search_Key(Tuple);							//��Ԫ������ȡ������						
int Get_Part_Key(Tuple);							//��Ԫ������ȡ�����
void Quick_Sort_Recursion(int*,Tuple*,int,int);		//��������ĵݹ麯��

/*-----��λ��չʹ��-----*/
void Exten_Index_Itself_Most();						//��ϣ��������չ����ҳ����
void Exten_Index_Page_Most();						//��ϣ��������չ��ҳ��֮��
void Bucket_Split_Most(int);						//Ͱ����
int Get_IndexID_Most(int,int);						//���Ԫ��Ķ�Ӧ����ֵ
int Exten_Decimal(int,int);							//ʮ������ʽ�µ���չ�����ڸ���Ͱ����

/*-----��λ��չʹ��-----*/							//���º�������ͬ��λ��չ
void Exten_Index_Itself_Least();
void Exten_Index_Page_Least();
void Bucket_Split_Least(int);
int Get_IndexID_Least(int,int);

#endif
