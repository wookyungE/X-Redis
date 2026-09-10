# 논문 그래프 출처

출처: Seungmin Shin, Leeju Kim, Wookyung Lee, Eyee Hyun Nam, Seungmin Kim, Bryan S. Kim, Sungjin Lee, Eunji Lee, **Avoiding Pitfalls in Networked Key-Value Store for Tiered Memory**, IEEE CLOUD 2025. © 2025 IEEE.

논문: https://doi.org/10.1109/CLOUD67622.2025.00050

| 파일 | 원본 | 설명 |
| --- | --- | --- |
| `oma-allocation-latency.png` | Figure 5, 인쇄 페이지 434 | OMA 적용 전후의 할당 지연 |
| `memtier-throughput.png` | Figure 8, 인쇄 페이지 437 | P를 기준으로 정규화한 SET/GET 처리량 |

저자가 제공한 논문 PDF에서 그림과 원본 캡션 영역을 PNG로 추출했습니다. 수치 추정, 데이터 재구성, 색상 변경은 하지 않았습니다. 이 저장소에서 새로 측정한 결과가 아닙니다.

추출 방법: `pdftoppm`으로 PDF의 5번째 및 8번째 페이지를 긴 변 3000 px로 렌더링하고, Pillow로 그림 영역을 잘랐습니다. 자르기 좌표(left, top, right, bottom)는 각각 `(1194, 261, 2070, 639)`, `(234, 261, 2070, 1086)`입니다.
