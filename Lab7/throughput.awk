BEGIN { 
 highest_packet_id = 0;
 data = 0;
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

 
 # only useful for small unicast where packet_id doesn't wrap. 
 # checking receive means avoiding recording drops 
 if ( action != "d" ) { 
 	if ( action == "r" && node_2 == 3.0 && (node_2_address == 3.0 || node_2_address == 3.1)) { 
		data = data + size ;
 		printf("%f %f\n",time,data/time);
 	} 
 }  
} 
END { 
 
}
