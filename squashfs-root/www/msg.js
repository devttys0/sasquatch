//public message
var msg_blank = "%s <%1%>";
var msg_space = "<%2%> %s\n";
var msg_blank_in = "<%3%> %s\n";
var msg_invalid = "\n<%4%> %s\n<%5%>: \n%s\n\n";
var msg_check_invalid = "%s <%6%>";
var msg_greater = "%s <%7%> %s";
var msg_less = "%s <%8%> %s";

function addstr(input_msg)
{
	var last_msg = "";
	var str_location;
	var temp_str_1 = "";
	var temp_str_2 = "";
	var str_num = 0;
	temp_str_1 = addstr.arguments[0];
	while(1)
	{
		str_location = temp_str_1.indexOf("%s");
		if(str_location >= 0)
		{
			str_num++;
			temp_str_2 = temp_str_1.substring(0,str_location);
			last_msg += temp_str_2 + addstr.arguments[str_num];
			temp_str_1 = temp_str_1.substring(str_location+2,temp_str_1.length);
			continue;
		}
		if(str_location < 0)
		{
			last_msg += temp_str_1;
			break;
		}
	}
	return last_msg;
}
