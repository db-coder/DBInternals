gnuplot <<EOF
	set xlabel "File Size"
	set ylabel "Time to construct B+ Tree"
	set title "Size VS Time"
	set term png
	set logscale x 10
	set output "analysis.png"
	plot "analysis.csv" using 1:2 with linespoints title "Top Down", \
		 "analysis.csv" using 1:4 with linespoints title "Top Down Sorted", \
		 "analysis.csv" using 1:6 with linespoints title "Bottom Up Sorted"
EOF

gnuplot <<EOF
	set xlabel "File Size"
	set ylabel "No of nodes in B+ Tree"
	set title "Size VS Nodes"
	set term png
	set logscale x 10
	set output "analysis1.png"
	plot "analysis.csv" using 1:3 with linespoints title "Top Down", \
		 "analysis.csv" using 1:5 with linespoints title "Top Down Sorted", \
		 "analysis.csv" using 1:7 with linespoints title "Bottom Up Sorted"
EOF