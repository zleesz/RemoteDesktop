#include <stdio.h>
#include "..\libpng16\png.h"

//////////////////////////////////////////////////////////////////////////

#define        FILE_ERROR        -1

class PngLoader
{
public:
	static inline long ReadPngData( const char *szPath, int *pnWidth, int *pnHeight, unsigned char **cbData )
	{
		FILE *fp = NULL;
		long file_size = 0, pos = 0;
		int color_type = 0, x = 0, y = 0, block_size = 0;

		png_infop info_ptr;
		png_structp png_ptr;
		png_bytep *row_point = NULL;

		fp = fopen( szPath, "rb" );
		if( !fp )    return FILE_ERROR;            //文件打开错误则返回 FILE_ERROR

		png_ptr  = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);        //创建png读取结构
		info_ptr = png_create_info_struct(png_ptr);        //png 文件信息结构
		png_init_io(png_ptr, fp);                //初始化文件 I\O
		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);                //读取png文件

		*pnWidth  = png_get_image_width( png_ptr, info_ptr );        //获得图片宽度
		*pnHeight = png_get_image_height( png_ptr, info_ptr );        //获得图片高度
		color_type = png_get_color_type( png_ptr, info_ptr );        //获得图片颜色类型
		file_size = (*pnWidth) * (*pnHeight) * 4;                    //计算需要存储RGB(A)数据所需的内存大小
		*cbData = (unsigned char *)malloc(file_size);            //申请所需的内存, 并将传入的 cbData 指针指向申请的这块内容

		row_point = png_get_rows( png_ptr, info_ptr );            //读取RGB(A)数据

		block_size = color_type == 6 ? 4 : 3;                    //根据是否具有a通道判断每次所要读取的数据大小, 具有Alpha通道的每次读4字节, 否则读3字节

		//将读取到的RGB(A)数据按规定格式读到申请的内存中
		for( x = 0; x < *pnHeight; x++ )
		{
			for( y = 0; y < *pnWidth*block_size; y+=block_size )
			{
				(*cbData)[pos++] = row_point[x][y + 2];        //B
				(*cbData)[pos++] = row_point[x][y + 1];        //G
				(*cbData)[pos++] = row_point[x][y + 0];        //R
				(*cbData)[pos++] = row_point[x][y + 3];        //Alpha
			}
		}
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		fclose( fp );
		return file_size;
	}
};