# !/bin/bash 

ssh deimos.lbl.gov 'mv ~/public_html/dom/release.*.gz ~/public_html/dom/old'
gzip -c release.hex | ssh deimos.lbl.gov 'cat > ~/public_html/dom/release.hex.gz'
gzip -c release.hex.0 | ssh deimos.lbl.gov 'cat > ~/public_html/dom/release.hex.0.gz'
gzip -c release.hex.1 | ssh deimos.lbl.gov 'cat > ~/public_html/dom/release.hex.1.gz'
