#include <stdio.h>
#include "map.h"

int main() {
    // 맵 초기화
    Map map;
    map_init(&map);

    // 키-값 쌍 추가
    map_put(&map, 1, 100);
    map_put(&map, 2, 200);
    map_put(&map, 3, 300);

    // 키에 해당하는 값 조회
    printf("Key: 1, Value: %d\n", map_get(&map, 1));
    printf("Key: 2, Value: %d\n", map_get(&map, 2));
    printf("Key: 3, Value: %d\n", map_get(&map, 3));

    // 존재하지 않는 키 조회
    printf("Key: 4, Value: %d\n", map_get(&map, 4));

    // 키-값 쌍 제거
    map_remove(&map, 2);

    // 제거된 키 조회
    printf("Key: 2, Value: %d\n", map_get(&map, 2));

    // 맵 해제
    map_free(&map);

    return 0;
}
