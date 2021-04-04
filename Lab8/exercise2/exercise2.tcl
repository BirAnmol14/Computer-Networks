set ns [new Simulator]
#simulation file
set nf [open out2.nam w]
$ns namtrace-all $nf
#trace file
set tf [open outall2.tr w]
$ns trace-all $tf
#cwnd file
set cf1 [open cwnd1.tr w]
set cf2 [open cwnd2.tr w]
#set color
$ns color 1 Blue
$ns color 2 Red
#designing architecture
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
$ns duplex-link $n0 $n1 2Mb 100ms DropTail
$ns duplex-link $n3 $n1 2Mb 100ms DropTail
$ns duplex-link $n1 $n2 2Mb 100ms DropTail

#finish procedure
proc finish { } {
	global ns nf tf cf1 cf2
	close $nf
	close $tf 
	close $cf1
	close $cf2
	exec nam out2.nam &
	exec awk -f throughput.awk < outall2.tr > throughput.tr
	#can add xgrpah as well
} 

# procedure to monitor current window
proc monitor-tcp-cwnd { src cf } {
	#the src here is the source's tcp object
	global ns
	set cwnd [ $src set cwnd_ ]
	set now [$ns now]
	#write to file x value , y value
	puts $cf "$now $cwnd"
	set time .1
	#set timer to recall this function
	$ns at [expr $now + $time] "monitor-tcp-cwnd $src $cf"
}
#set n0
set tcp [new Agent/TCP]
$tcp set class_ 1
$ns attach-agent $n0 $tcp
$tcp set packet_size_ 500
set ftp [new Application/FTP]
$ftp attach-agent $tcp

#set n3
set tcp1 [new Agent/TCP]
$tcp1 set class_ 2
$ns attach-agent $n3 $tcp1
$tcp1 set packet_size_ 500
set ftp1 [new Application/FTP]
$ftp1 attach-agent $tcp1
#set sink
set sink0 [new Agent/TCPSink]
set sink1 [new Agent/TCPSink]
$ns attach-agent $n2 $sink0
$ns attach-agent $n2 $sink1

#connect src and sink
$ns connect $tcp $sink0
$ns connect $tcp1 $sink1 
#set src
set src $ftp
set src1 $ftp1
#set simulation
$ns at 1 "$src start"
$ns at 1 "monitor-tcp-cwnd $tcp $cf1"
$ns at 10 "$src1 start"
$ns at 10 "monitor-tcp-cwnd $tcp1 $cf2"
$ns at 30 "$src stop"
$ns at 30 "$src1 stop"
$ns at 30 "finish"

#run simulation
$ns run


