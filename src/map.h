#ifndef MAP_H
#define MAP_H

// 맵의 키-값 쌍을 나타내는 구조체
typedef struct {
    int key;
    int value;
} MapEntry;

// 맵 구조체
typedef struct {
    MapEntry* entries;
    int size;
    int capacity;
} Map;

// 맵 초기화 함수
void map_init(Map* map);

// 맵에 키-값 쌍 추가 함수
void map_put(Map* map, int key, int value);

// 맵에서 키에 해당하는 값 조회 함수
int map_get(Map* map, int key);

// 맵에서 키에 해당하는 키-값 쌍 제거 함수
void map_remove(Map* map, int key);

// 맵 해제 함수
void map_free(Map* map);

#endif

