BEGIN {
	cnt = 0;
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
	
	if ( action == "d" && type == "tcp") { 
		cnt+=1;
	}
 
}

END{
	printf("Dropped TCP pkt cnt: %d",cnt);
}

