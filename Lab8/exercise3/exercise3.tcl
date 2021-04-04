set ns [new Simulator]
#simulation file
set nf [open out2.nam w]
$ns namtrace-all $nf
#trace file
set tf [open outall2.tr w]
$ns trace-all $tf
#cwnd file
set cf1 [open cwnd1.tr w]

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
$ns duplex-link $n1 $n2 1.5Mb 100ms DropTail

#finish procedure
proc finish { } {
	global ns nf tf cf1 
	close $nf
	close $tf 
	close $cf1
	
	exec nam out2.nam &
	#throughtput at n2
	exec awk -f throughput.awk < outall2.tr > throughput.tr &
	#throughtput at n0
	exec awk -f throughput1.awk < outall2.tr > throughput1.tr &
	#throughtput at n3
	exec awk -f throughput2.awk < outall2.tr > throughput2.tr &
	#avg throughput
	exec awk -f avgThroughput.awk < outall2.tr > avgThroughput.txt &
	#pkloss
	#tcp
	exec awk -f  pktloss.awk < outall2.tr > tcpLoss.tr &
	#udp 
	exec awk -f  pkLossUdp.awk < outall2.tr > udpLoss.tr &
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
$tcp set packet_size_ 1000
set ftp [new Application/FTP]
$ftp attach-agent $tcp

#set n3
set udp [new Agent/UDP]
$udp set class_ 2
$ns attach-agent $n3 $udp
set traffic [new Application/Traffic/CBR]
$traffic set packetSize_ 800
$traffic set interval_ 0.005
$traffic attach-agent $udp
#set sink
set sink0 [new Agent/TCPSink]
set sink1 [new Agent/LossMonitor]
$ns attach-agent $n2 $sink0
$ns attach-agent $n2 $sink1

#connect src and sink
$ns connect $tcp $sink0
$ns connect $udp $sink1
#set src
set src $ftp
set src1 $traffic
#set simulation
$ns at 0.1 "$src1 start"
$ns at 1 "$src start"
$ns at 1 "monitor-tcp-cwnd $tcp $cf1"
$ns at 30 "$src stop"
$ns at 30 "$src1 stop"
$ns at 30 "finish"

#run simulation
$ns run


