# !/bin/bash 

mv ~/public_html/dom/release.*.gz ~/public_html/dom/old
gzip -c release.hex > ~/public_html/dom/release.hex.gz
gzip -c release.hex.0 > ~/public_html/dom/release.hex.0.gz
gzip -c release.hex.1 > ~/public_html/dom/release.hex.1.gz
