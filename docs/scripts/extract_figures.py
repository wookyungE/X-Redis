"""Extract vector figures from the author-supplied paper PDF.

Usage: python extract_figures.py /path/to/paper.pdf
Requires PyMuPDF and Poppler's pdftocairo. No rasterization is performed.
"""
import pathlib
import subprocess
import sys

import pymupdf

OUTPUT = pathlib.Path(__file__).resolve().parents[1] / "figures"
# PDF page numbers are one-based; crop rectangles use PDF points from top left.
FIGURES = [
    ("oma-allocation-latency", 5, (315, 69, 546, 152)),
    ("memtier-throughput", 8, (62, 69, 547, 263)),
    ("twitter-throughput", 10, (62, 69, 541, 148)),
    ("twitter-value-distribution", 10, (62, 180, 541, 264)),
]


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    with pymupdf.open(sys.argv[1]) as source:
        for name, page_number, bounds in FIGURES:
            clip = pymupdf.Rect(bounds)
            pdf_path = OUTPUT / f"{name}.pdf"
            with pymupdf.open() as extracted:
                page = extracted.new_page(width=clip.width, height=clip.height)
                page.show_pdf_page(page.rect, source, page_number - 1, clip=clip)
                extracted.save(pdf_path, garbage=4, deflate=True)
            subprocess.run(
                ["pdftocairo", "-svg", str(pdf_path), str(OUTPUT / f"{name}.svg")],
                check=True,
            )


if __name__ == "__main__":
    main()
