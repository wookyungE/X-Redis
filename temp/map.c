#include <stdlib.h>
#include "map.h"

void map_init(Map* map) {
    map->entries = NULL;
    map->size = 0;
    map->capacity = 0;
}

void map_put(Map* map, int key, int value) {
    // 새로운 키-값 쌍 생성
    MapEntry entry;
    entry.key = key;
    entry.value = value;

    // 맵의 크기가 용량을 초과하는 경우, 용량을 늘림
    if (map->size >= map->capacity) {
        map->capacity = (map->capacity == 0) ? 1 : map->capacity * 2;
        map->entries = realloc(map->entries, map->capacity * sizeof(MapEntry));
    }

    // 키-값 쌍을 맵에 추가
    map->entries[map->size++] = entry;
}

int map_get(Map* map, int key) {
    // 키에 해당하는 값 조회
    for (int i = 0; i < map->size; i++) {
        if (map->entries[i].key == key) {
            return map->entries[i].value;
        }
    }
    // 키가 존재하지 않는 경우, 0 반환 또는 에러 처리 등을 할 수 있음
    return 0;
}

void map_remove(Map* map, int key) {
    // 키에 해당하는 키-값 쌍 제거
    for (int i = 0; i < map->size; i++) {
        if (map->entries[i].key == key) {
            // 삭제된 키-값 쌍 이후의 요소들을 앞으로 이동
            for (int j = i; j < map->size - 1; j++) {
                map->entries[j] = map->entries[j + 1];
            }
            // 맵의 크기 감소
            map->size--;
            break;
        }
    }
}

void map_free(Map* map) {
    // 맵의 메모리 해제
    free(map->entries);
    map->entries = NULL;
    map->size = 0;
    map->capacity = 0;
}

