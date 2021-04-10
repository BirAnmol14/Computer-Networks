set ns [new Simulator]
set nf [open out.nam w]
$ns namtrace-all $nf
set tf [open outall.tr w]
$ns trace-all $tf
set cong [open congestion.xg w]

$ns color 1 Blue
$ns color 2 Red
$ns color 3 Green
#Sesion Routing
$ns rtproto Session
#nodes
set n0 [$ns node]
set n1 [$ns node] 
set n2 [$ns node]
set n3 [$ns node] 
set n4 [$ns node] 
set n5 [$ns node] 
set n6 [$ns node] 
set n7 [$ns node] 

$ns duplex-link $n0 $n2 1Mb 2ms DropTail
$ns duplex-link $n1 $n2 1Mb 2ms DropTail
$ns duplex-link $n2 $n3 700Kb 2ms DropTail
$ns duplex-link $n4 $n3 1Mb 2ms DropTail
$ns duplex-link $n3 $n5 700Kb 2ms DropTail
$ns duplex-link $n5 $n6 1Mb 2ms DropTail
$ns duplex-link $n5 $n7 1Mb 2ms DropTail

proc finish {} {
	global ns nf tf cong
	$ns flush-trace
	close $nf
	close $tf
	close $cong
	exec nam out.nam &
	exec awk -f dropped.awk < outall.tr > dropped.xg &
	exec awk -f dropped_cnt.awk < outall.tr > dropped_cnt.txt &
	exit 0 
}

proc plotWindow { src fp } {
	#the src here is the source's tcp object
	global ns
	set cwnd [ $src set cwnd_ ]
	set now [$ns now]
	#write to file x value , y value
	puts $fp "$now $cwnd"
	set time .1
	#set timer to recall this function
	$ns at [expr $now + $time] "plotWindow $src $fp"
}

proc ftp-agent { node sink size class } {
	global cong
	set ns [Simulator instance]
	set tcp [new Agent/TCP]
	$tcp set class_ $class
	$ns attach-agent $node $tcp
	$tcp set packet_size_ $size
	set ftp [new Application/FTP]
	$ftp attach-agent $tcp
	$ns connect $tcp $sink
	$ns at 1.0 "plotWindow $tcp $cong"
	return $ftp
}
proc cbr-agent { node sink size interval class } {
	set ns [Simulator instance]
	set udp [new Agent/UDP]
	$ns attach-agent $node $udp
	$udp set class_ $class
	set traffic [new Application/Traffic/CBR]
	$traffic set packetSize_ $size
	$traffic set interval_ $interval
	$traffic attach-agent $udp
	$ns connect $udp $sink
	return $traffic
}

set sink0 [new Agent/TCPSink]
set sink1 [new Agent/LossMonitor]
set sink2 [new Agent/LossMonitor]
$ns attach-agent $n6 $sink0
$ns attach-agent $n7 $sink1
$ns attach-agent $n7 $sink2
#TCP
set src0 [ftp-agent $n1 $sink0 500 1]
#UDP
set src1 [cbr-agent $n0 $sink1 500 0.005 2]
set src2 [cbr-agent $n4 $sink2 500 0.005 3]

$ns at 1.0 "$src0 start"
$ns at 8.0 "$src1 start"
$ns at 8.0 "$src2 start"
$ns at 13.0 "$src1 stop"
$ns at 13.0 "$src2 stop"
$ns at 19.0 "$src0 stop"
$ns at 20.0 "finish"
$ns run

