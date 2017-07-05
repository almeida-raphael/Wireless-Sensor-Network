/*
 * Example Contiki test script (JavaScript).
 * A Contiki test script acts on mote output, such as via printf()'s.
 * The script may operate on the following variables:
 *  Mote mote, int id, String msg
 */

TIMEOUT(1800000, log.log("Timeout!!!"));

var simTimeStamp = +(new Date)

 //import Java Package to JavaScript
 importPackage(java.io);
 
 // Use JavaScript object as an associative array
 outputs = new Object();
 
 while (true) {
 	//Has the output file been created.
 	if(! outputs[id.toString()]){
 		// Open log_<id>.txt for writing.
 		// BTW: FileWriter seems to be buffered.
 		outputs[id.toString()]= new FileWriter("/media/user/Files/log_" + id +"_"+ simTimeStamp +".txt");
 	}
 	//Write to file.
 	outputs[id.toString()].write(time + " " + msg + "\n");
   
 	try{
 		//This is the tricky part. The Script is terminated using
 		// an exception. This needs to be caught.
 		YIELD();
 	} catch (e) {
 		//Close files.
 		for (var ids in outputs){
 			outputs[ids].close();
 		}
 		//Rethrow exception again, to end the script.
 		throw('test script killed');
 	}
 }