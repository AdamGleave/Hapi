wget -r -np -nd -A '*.gz' -e robots=off http://lime.cs.elte.hu/~kpeter/data/mcf/vision/
for fname in *.gz; do
  gunzip $fname
done
