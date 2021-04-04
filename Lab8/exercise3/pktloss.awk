BEGIN {
	highest_packet_id = 0;
	tcpPkt = 0;
	UdpPkt = 0;
}

{
	action = $1; 
 	time = $2; 
 	node_1 = $3; 
 	node_2 = $4; 
 	type = $5; 
 	size =$6;
 	flow_id = $8; 
 	node_1_address = $9; 
 	node_2_address = $10; 
 	seq_no = $11; 
 	packet_id = $12;
	
	if ( packet_id > highest_packet_id ) highest_packet_id = packet_id; 

	if ( action == "d" ) { 
 	if ( (node_2_address == 2.0 ) && node_1_address == 0.0) { 
		#tcp
		tcpPkt = tcpPkt + 1
 		printf("%f %f\n",time,tcpPkt);
 	} 
	
 }  
}

END{

}
