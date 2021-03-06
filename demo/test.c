#include "cson.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

/**
 * 该示例会使用cson解析如下所示播放列表。
 *
{
    "name":"jay zhou",
    "creater":"dahuaxia",
    "songNum":2,
    "songList":[
        {
            "songName":"qilixiang",
            "signerName":"jay zhou",
            "albumName":"qilixiang",
            "url":"www.kugou.com",
            "duration":200,
            "paid":false,
            "price":6.6600000000000001,
            "lyricNum":2,
            "lyric":[
                {
                    "time":1,
                    "text":"Sparrow outside the window"
                },
                {
                    "time":10,
                    "text":"Multi mouth on the pole"
                }
            ],
            "key":[
                1234,
                5678,
                9876
            ]
        },
        {
            "songName":"dongfengpo",
            "signerName":"jay zhou",
            "albumName":"dongfengpo",
            "url":"music.qq.com",
            "duration":180,
            "paid":true,
            "price":0.88,
            "lyricNum":2,
            "lyric":[
                {
                    "time":10,
                    "text":"A sad parting, standing alone in the window"
                },
                {
                    "time":20,
                    "text":"I'm behind the door pretending you're not gone"
                }
            ],
            "key":[
                1111,
                2222,
                3333
            ]
        }
    ],
    "extData":{
        "a":999,
        "b":1.05
    }
}
*/

/**
 * 1）首先我们需要定义与上面json相对应的数据结构。于是有了@PlayList、@ExtData、@SongInfo、@Lyric。
 *    即使不实用cson，想要解析json，通常你也需要这么做。
 *
 *    注意:结构体属性名需与json中字段名一致;
 *    注意:当某个字段在json中被定义为数组时，那么该字段在结构体中要被声名为指针，并且增加数组size的字段。
 *
 * 2）为了C语言能够像java中通过反射来操作结构体中的属性，我们需要先为每个结构体定义一个用于查找结构体属性的“反射表”，
 *    即play_list_ref_tbl，ext_data_ref_tbl，song_ref_tbl，lyric_ref_tbl。
 *    有了这个反射表，我们可以迭代访问数组元素。不仅可以帮助我们完成json解析，当我们想要输出对象各属性值、或是释放
 *    指针指向的堆内存时也很有用。
 *
 *    TODO:目前反射表的结构有些复杂，虽然提供了宏定义让它用来稍稍方便一些。需要对该结构做出优化。
 *
 * 3）正确的完成上面两步，解析工作其实基本上就要完成了。只要再调用csonJsonStr2Struct，所有的属性就都会正确的赋值到结构体。
 *
 */


/*
    Step1:定义与json相对应的数据结构
*/
typedef struct {
    int         time;
    char*       text;
} Lyric;

typedef struct {
    char*       songName;
    char*       signerName;
    char*       albumName;
    char*       url;
    int         duration;
    int         paid;
    double      price;
    size_t      lyricNum;
    Lyric*      lyric;
    size_t      keyNum;
    int*        key;
    size_t      strNum;
    char**      strList;
} SongInfo;

typedef struct {
    int         a;
    double      b;
} ExtData;

typedef struct {
    char*       name;
    char*       creater;
    size_t      songNum;
    SongInfo*   songList;
    ExtData     extData;
} PlayList;

/*
    Step2:定义数据结构的反射表
*/
reflect_item_t lyric_ref_tbl[] = {
    _property_int(Lyric, time),
    _property_string(Lyric, text),
    _property_end()
};

reflect_item_t song_ref_tbl[] = {
    _property_string(SongInfo, songName),
    _property_string(SongInfo, signerName),
    _property_string(SongInfo, albumName),
    _property_string(SongInfo, url),
    _property_int(SongInfo, duration),
    _property_bool(SongInfo, paid),
    _property_real(SongInfo, price),
    _property_int(SongInfo, lyricNum),
    _property_array_object(SongInfo, lyric, lyric_ref_tbl, Lyric, lyricNum),
    _property_int(SongInfo, keyNum),
    _property_array_int(SongInfo, key, int, keyNum),
    _property_int(SongInfo, strNum),
    _property_array_string(SongInfo, strList, char*, strNum),
    _property_end()
};

reflect_item_t ext_data_ref_tbl[] = {
    _property_int(ExtData, a),
    _property_real(ExtData, b),
    _property_end()
};

reflect_item_t play_list_ref_tbl[] = {
    _property_string(PlayList, name),
    _property_string(PlayList, creater),
    _property_int(PlayList, songNum),
    _property_array_object(PlayList, songList, song_ref_tbl, SongInfo, songNum),
    _property_obj(PlayList, extData, ext_data_ref_tbl),
    _property_end()
};

void printPlayList(PlayList* list);
void freePlayList(PlayList* list);

const static char* jStr = "{\"name\":\"jay zhou\",\"creater\":\"dahuaxia\",\"songList\":[{\"songName\":\"qilixiang\",\"signerName\":\"jay zhou\",\"albumName\":\"qilixiang\",\"url\":\"www.kugou.com\",\"duration\":20093999939292928292234.1,\"paid\":false,\"price\":6.66,\"lyric\":[{\"time\":1,\"text\":\"Sparrow outside the window\"},{\"time\":10,\"text\":\"Multi mouth on the pole\"}],\"key\":[1111,2222,3333]},{\"songName\":\"dongfengpo\",\"signerName\":\"jay zhou\",\"albumName\":\"dongfengpo\",\"url\":\"music.qq.com\",\"duration\":180.9,\"paid\":true,\"price\":0.88,\"lyric\":[{\"time\":10,\"text\":\"A sad parting, standing alone in the window\"},{\"time\":20,\"text\":\"I'm behind the door pretending you're not gone\"}],\"key\":[1234,5678,9876],\"strList\":[\"abcd\",\"efgh\",\"ijkl\"]}],\"extData\":{\"a\":999,\"b\":1}}";

/*
    Step3:调用csonJsonStr2Struct/csonStruct2JsonStr实现反序列化和序列化
*/
void test1()
{
    printf("=========================================\n");
    printf("\t\tRunning %s\n", __FUNCTION__);
    printf("=========================================\n");
    PlayList playList;

    /* string to struct */
    int ret = csonJsonStr2Struct(jStr, &playList, play_list_ref_tbl);
    /* test print */
    printf("ret=%d\n", ret);
    printPlayList(&playList);

    char* jstrOutput;
    ret = csonStruct2JsonStr(&jstrOutput, &playList, play_list_ref_tbl);
    printf("ret=%d\nJson:%s\n", ret, jstrOutput);
    free(jstrOutput);
    freePlayList(&playList);
}

void* printProperty(void* pData, const reflect_item_t* tbl)
{
    if (tbl->type == CSON_ARRAY || tbl->type == CSON_OBJECT) return NULL;

    if (tbl->type == CSON_INTEGER || tbl->type == CSON_TRUE || tbl->type == CSON_FALSE) printf("%s:%d\n", tbl->field, *(int*)pData);

    if (tbl->type == CSON_REAL) printf("%s:%f\n", tbl->field, *(double*)pData);

    if (tbl->type == CSON_STRING) printf("%s:%s\n", tbl->field, *((char**)pData));

    return NULL;
}

void* freePointer(void* pData, const reflect_item_t* tbl)
{
    if (tbl->type == CSON_ARRAY || tbl->type == CSON_STRING) {
        printf("free field %s.\n", tbl->field);
        free(*(void**)pData);
    }
    return NULL;
}

void printPlayList(PlayList* list)
{
    /* 调用loopProperty迭代结构体中的属性,完成迭代输出属性值 */
    csonLoopProperty(list, play_list_ref_tbl, printProperty);
}

void freePlayList(PlayList* list)
{
    /* 调用loopProperty迭代结构体中的属性,释放字符串和数组申请的内存空间 */
    csonLoopProperty(list, play_list_ref_tbl, freePointer);
}
