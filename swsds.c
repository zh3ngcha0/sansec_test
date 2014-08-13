﻿# include "mysansec.h"




#define MAX_PTHREAD  (1024 * 3)

SGD_HANDLE hDeviceHandle; /*全局设备句柄    typedef void*    SGD_HANDLE;*/


//声明为外部变量以便于测试函数使用
void *hSessionHandle;
unsigned int algorithm_id = SGD_SM2_3;  // 指定算法标识
unsigned int key_len = 256;
ECCrefPublicKey public_key;
ECCrefPrivateKey private_key;

void *encrypt_func(void *argv);  // 对一个文件的加密线程
void *decrypt_func(void *argv);

int main(int argc, char **argv)
{
	printf("%d \n", argc);
	if(argc <= 2)
	{
		printf("______argv erro \n");
		printf("argv[1]--> Encryption thread number, argv[2]\n");
		return -1;
	}


	struct timeval main_begin_time, main_stop_time;
		gettimeofday(&main_begin_time, NULL);
		printf("main_stop_time--> %ld \n", main_stop_time.tv_usec);



	char *num_ch = argv[1];
	int test_num = atoi(num_ch);

	printf("%s, %d \n",num_ch, test_num);

	struct _Head_File_Mutex *head = NULL;
	if(NULL == (head = init_head_file_mutex(test_num)))
	{
		printf("头节点初始化失败\n");
		return -1;
	}

	printf("-----------------\n");

	struct _List_File_Mutex *node = NULL; // 文件节点
	// 初始化test_num个明文文件，并加入链表结构
	int i;
	for(i = 1; i <= test_num; i++)
	{
		if(NULL == (node = make_test_file_and_init_list_file_mutex_node_and_ret(i)))  // 生成文件信息节点
		{
			printf("内存申请失败\n");
			return 0;
		}
		add_node_to_list(head, node);
	}

	int ret;
	if (SDR_OK != (ret = SDF_OpenDevice(&hDeviceHandle))) {
		print_error_msg(ret, "打开设备失败");
		return ret;
	}

	if (SDR_OK != (ret = SDF_OpenSession(hDeviceHandle, &hSessionHandle))) {
		print_error_msg(ret, "打开会话失败");
		return ret;
	}


	//获取指定长度的随机数
	unsigned int random_length = 256;
	unsigned char random_out_buffer[16384] = {0}; // ?需要否

	get_generate_random(hSessionHandle, random_length, random_out_buffer);

	PrintData("随机数:", random_out_buffer, random_length, 64);  //一行打印64个八进制数



	generate_key_pair_ecc(hSessionHandle, algorithm_id, key_len, &public_key, &private_key);

	save_key_pair_ecc(&public_key, &private_key); // 保存密钥对到文件


	pthread_t pid_encrypt[MAX_PTHREAD];  //MAX_PTHREAD 加密线程
	pthread_t pid_decrypt[MAX_PTHREAD];  //MAX_PTHREAD 解密线程



	create_encrypt_pthread(head, &pid_encrypt);  // 创建加密线程，并且加入了等待线程结束的代码

	create_decrypt_pthread(head, &pid_decrypt); // 创建解密线程，并且加入了等待线程结束的代码



	printf("=======对数据加密成功====\n");
	gettimeofday(&main_stop_time, NULL);
	printf("main_stop_time--> %ld \n",main_stop_time.tv_usec);
	printf("main_begin_time--> %ld \n",main_begin_time.tv_usec);
	printf("%d个文件 加解密用时----> %ld \n",test_num, main_stop_time.tv_usec - main_begin_time.tv_usec);



	close_devices_and_session(hDeviceHandle, hSessionHandle);
	destroy_list(head);
	return 0;
}