BEGIN { 
 highest_packet_id = 0; 
} 
 
{ 
 action = $1; 
 time = $2; 
 node_1 = $3; 
 node_2 = $4; 
 src = $5; 
 flow_id = $8; 
 node_1_address = $9; 
 node_2_address = $10; 
 seq_no = $11; 
 packet_id = $12; 
 if ( packet_id > highest_packet_id ) highest_packet_id = packet_id; 
 
 if(node_1 == 1 && node_1_address == 1.0 && node_2_address == 3.0){
	if ( start_time[packet_id] == 0 ) start_time[packet_id] = time; 
 }
 
 if ( action != "d" ) { 
 if ( action == "r" && node_2 ==3 && node_1_address == 1.0 && node_2_address == 3.0) { 
	end_time[packet_id] = time; 
 } 
 } else { 
	end_time[packet_id] = -1; 
 } 
} 
END { 
 for ( packet_id = 0; packet_id <= highest_packet_id; packet_id++ ) { 
	start = start_time[packet_id]; 
	end = end_time[packet_id]; 
	packet_duration = end - start; 
	if ( start < end ) printf("%f %f\n", start, packet_duration); 
 } 
}
