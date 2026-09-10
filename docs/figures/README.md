# Paper figure sources

Source: Seungmin Shin, Leeju Kim, Wookyung Lee, Eyee Hyun Nam, Seungmin Kim, Bryan S. Kim, Sungjin Lee, Eunji Lee, Avoiding Pitfalls in Networked Key-Value Store for Tiered Memory, IEEE CLOUD 2025. © 2025 IEEE.

[Paper DOI](https://doi.org/10.1109/CLOUD67622.2025.00050)

| Figure | Original location | SVG | Vector PDF |
| --- | --- | --- | --- |
| Allocation latency with and without OMA | Figure 5, printed page 434 | [SVG](oma-allocation-latency.svg) | [PDF](oma-allocation-latency.pdf) |
| SET/GET throughput normalized to P | Figure 8, printed page 437 | [SVG](memtier-throughput.svg) | [PDF](memtier-throughput.pdf) |
| Twitter workload throughput normalized to P | Figure 10, printed page 439 | [SVG](twitter-throughput.svg) | [PDF](twitter-throughput.pdf) |
| Twitter value-size distributions | Figure 11, printed page 439 | [SVG](twitter-value-distribution.svg) | [PDF](twitter-value-distribution.pdf) |

These files preserve the vector graphics embedded in the author-supplied paper PDF. The original standalone plotting files were not available. Each figure region was extracted into a vector PDF using PyMuPDF, then converted to SVG with Poppler's `pdftocairo`. No screenshot, rasterization, data estimation, or replotting was used. SVG text is represented by vector glyph paths to preserve its appearance without requiring the original fonts. Captions and attribution are provided in the main README.

The figures show published results, not new measurements from this checkout. Their original colors, axes, labels, and error bars are retained.

## Reproduce the extraction

Install PyMuPDF in a Python environment and ensure Poppler's `pdftocairo` is available, then run from the repository root:

```bash
python docs/scripts/extract_figures.py /path/to/Avoiding_Pitfalls_in_Networked_Key-Value_Store_for_Tiered_Memory.pdf
```

The [extraction script](../scripts/extract_figures.py) records the source pages and crop rectangles in PDF points. The full paper PDF is not included in this repository.
