/*
 * TODO: Make into a proper singleton
 * FIXME: Initilization Bug
 * TODO: Implement SWIPES and other events see: TouchEvent
*/
package flash.events {		
	import flash.display.Stage;
	import flash.display.Sprite;	
	import flash.display.DisplayObject;
	import flash.display.DisplayObjectContainer;
	import flash.events.Event;		
	import flash.events.DataEvent;
	import flash.events.MouseEvent;
	import flash.events.IOErrorEvent;
	import flash.events.ProgressEvent;
	import flash.events.SecurityErrorEvent;
	import flash.geom.Point;	
	import flash.net.URLRequest;
	import flash.net.Socket;
	import flash.utils.getTimer;
	import flash.utils.ByteArray;
	import flash.utils.Endian;
	
	public class TUIO{	
		//private static var INSTANCE:TUIO;			
		private static var STAGE:Stage;
		private static var SOCKET:Socket;	
		private static var SOCKET_STATUS:Boolean;	
		private static var HOST:String;			
		private static var PORT:Number;				
		private static var FRAME_COUNT:int;
		//-------------------------------------- DEBUG VARS			
		internal static var DEBUG:Boolean;
		private static var DEBUGGER:TUIODebugger;
		private static var INITIALIZED:Boolean;		
		//--------------------------------------		
		private static var ID_ARRAY:Array; 		
		private static var EVENT_ARRAY:Array;		
		private static var OBJECT_ARRAY:Array;		
		//--------------------------------------
		private static var SWIPE_THRESHOLD:Number = 1000;
		private static var HOLD_THRESHOLD:Number = 4000;
			
//---------------------------------------------------------------------------------------------------------------------------------------------	
// CONSTRUCTOR - SINGLETON
//---------------------------------------------------------------------------------------------------------------------------------------------		
		public static function init ($STAGE:DisplayObjectContainer, $HOST:String, $PORT:Number, $PLAYBACK_URL:String, $DEBUG:Boolean = true):void{ //playback taken out of class but but param kept for backward compatability
			if (INITIALIZED) { trace("TUIO Already Initialized!"); return; }
					
			// FIXME: VERIFY STAGE IS AVAILABLE
			STAGE = $STAGE.stage;
			STAGE.align = "TL";
			STAGE.scaleMode = "noScale";				
			
			HOST=$HOST;			
			PORT=$PORT;					       
				
			DEBUG = $DEBUG;				
			INITIALIZED = true;
			OBJECT_ARRAY = new Array();
			ID_ARRAY = new Array();
			EVENT_ARRAY = new Array();
			
			if(DEBUG){
				activateDebugMode();						
			}  
			
			startSocket(null);				
			socketStatus(null);						
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
// PUBLIC METHODS
//---------------------------------------------------------------------------------------------------------------------------------------------
/*
	public static function addEventListener(e:EventDispatcher):void
		{
			EVENT_ARRAY.push(e);
		}
*/
//---------------------------------------------------------------------------------------------------------------------------------------------
		public static function addObjectListener(id:int, receiver:Object):void{
			var tmpObj:TUIOObject = getObjectById(id);			
			if(tmpObj){
				tmpObj.addListener(receiver);				
			}
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
		public static function removeObjectListener(id:int, receiver:Object):void{
			var tmpObj:TUIOObject = getObjectById(id);			
			if(tmpObj){
				tmpObj.removeListener(receiver);				
			}
		}		
//---------------------------------------------------------------------------------------------------------------------------------------------
		public static function getObjectById(id:int):TUIOObject{
			if(id == 0){
				return new TUIOObject("mouse", 0, STAGE.mouseX, STAGE.mouseY, 0, 0, 0, 0, 10, 10, null);
			}
			for each(var tuioObj:TUIOObject in OBJECT_ARRAY){
				if(tuioObj.ID == id){
					return tuioObj;
				}
			}
			return null;
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
		public static function startSocket(e:Event=null):void{ 
			try{			
				SOCKET = new Socket();	
				SOCKET.addEventListener(Event.CLOSE, closeHandler);
				SOCKET.addEventListener(Event.CONNECT, connectHandler);
				SOCKET.addEventListener(ProgressEvent.SOCKET_DATA, dataHandler);
				SOCKET.addEventListener(IOErrorEvent.IO_ERROR, ioErrorHandler);
				SOCKET.addEventListener(SecurityErrorEvent.SECURITY_ERROR, securityErrorHandler);	
				SOCKET.connect(HOST, PORT);
			}catch (e:Error) { }		
		}
		public static function stopSocket(e:Event=null):void{ 	
			try{			
				SOCKET.close();
			}catch (e:Error) { }	
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
// PRIVATE METHODS
//---------------------------------------------------------------------------------------------------------------------------------------------
		private static function processMessage(byteArray:ByteArray):void{
			byteArray.endian = Endian.LITTLE_ENDIAN;
			
			var aliveMessage:Boolean,
			    aliveBlobs:ByteArray;
						
			while(byteArray.bytesAvailable>=20){ // reading 12 for the path and 8 for the type (12 bytes + 8 bytes = 20 bytes)
				var path:String = byteArray.readUTFBytes(12),
				    type:String = byteArray.readUTFBytes(8);

				if(path === "/tuio/2Dcur"){
					if(type === "alive"){
						aliveMessage = true; //is this neccessary? should I just process every alive event instead of one per package?

						var numBlobs:int = byteArray.readInt(),
						    setMsgByteLen:int = (numBlobs-1) << 2;
						
						aliveBlobs = null;
						aliveBlobs = new ByteArray();
						
						if(numBlobs>1) byteArray.readBytes(aliveBlobs,0,setMsgByteLen);
					}else if(type === "set"){
						var id:int = byteArray.readInt(),
							x:Number = byteArray.readFloat()*STAGE.stageWidth,
							y:Number = byteArray.readFloat()*STAGE.stageHeight,
							X:Number = byteArray.readFloat(),
							Y:Number = byteArray.readFloat(),
							m:Number = byteArray.readFloat(),
							wd:Number = byteArray.readFloat(),
							ht:Number = byteArray.readFloat(),							
							stagePoint:Point = new Point(x,y),
							displayObjArray:Array = STAGE.getObjectsUnderPoint(stagePoint),
							dobj:DisplayObject = null,
							tuioobj:TUIOObject = getObjectById(id);
											
						if(displayObjArray.length > 0){
							dobj = displayObjArray[displayObjArray.length-1];	
						}
																							
						if(tuioobj == null){
							tuioobj = new TUIOObject("2Dcur", id, x, y, X, Y, -1, 0, wd, ht, dobj);
							STAGE.addChild(tuioobj.TUIO_CURSOR);								
							OBJECT_ARRAY.push(tuioobj);
							tuioobj.notifyCreated();
						}else{
							tuioobj.TUIO_CURSOR.x = x;
							tuioobj.TUIO_CURSOR.y = y;
							tuioobj.oldX = tuioobj.x;
							tuioobj.oldY = tuioobj.y;
							tuioobj.x = x;
							tuioobj.y = y;
							tuioobj.width = wd;
							tuioobj.height = ht;
							tuioobj.area = wd * ht;								
							tuioobj.dX = X;
							tuioobj.dY = Y;
							tuioobj.setObjOver(dobj);
							
							var d:Date = new Date();																
							
							if(!( int(X*1000) == 0 && int(Y*1000) == 0)){
								tuioobj.notifyMoved();
							}
							
							if( int(X*250) == 0 && int(Y*250) == 0) {
								var lastModDur:Number = d.time - tuioobj.lastModifiedTime;
								if(lastModDur < 0 ? -lastModDur : lastModDur > HOLD_THRESHOLD){
									var eventArrayLen:int = EVENT_ARRAY.length;
									for(var ndx:int=0; ndx<eventArrayLen; ndx++){
										EVENT_ARRAY[ndx].dispatchEvent(tuioobj.getTouchEvent(TouchEvent.LONG_PRESS));
									}
									tuioobj.lastModifiedTime = d.time;																		
								}
									
							}
						}
						try{
							if(tuioobj.TUIO_OBJECT && tuioobj.TUIO_OBJECT.parent){							
								var localPoint:Point = tuioobj.TUIO_OBJECT.parent.globalToLocal(stagePoint);							
								tuioobj.TUIO_OBJECT.dispatchEvent(new TouchEvent(TouchEvent.MOUSE_MOVE, true, false, x, y, localPoint.x, localPoint.y, tuioobj.oldX, tuioobj.oldY, tuioobj.TUIO_OBJECT, false,false,false, true, m, "2Dcur", id, 0, 0));
							}
						} catch (e:Error){
							trace("(" + e + ") Dispatch event failed " + tuioobj.ID);
						}

						//trace("[SET]",id,x,y,X,Y,m,wd,ht);
					}else if(type === "fseq"){
						FRAME_COUNT = byteArray.readInt();
						//trace("[FSEQ]",FRAME_COUNT);
					}
				}else if(path === "/tuio/2Dobj"){
					if(type === "alive"){				
						var numObjs:int = byteArray.readInt(),
						    aliveMsgByteLen = (numObjs-1) << 2;
						
						byteArray.position += aliveMsgByteLen;
					}else if(type === "set"){
						//account for set
					}else if(type === "fseq"){
						//do fseq events exist in 2DObj profiles?
					}
				}else{
					if(byteArray.bytesAvailable>0) byteArray.position++; //keep adding bytes one by one if no message type found
				}
			}
			
			if(aliveMessage){
				if(DEBUG && DEBUGGER.on){
					DEBUGGER.clearDebugText();
				}
				
				for each (var obj1:TUIOObject in OBJECT_ARRAY){
					obj1.TUIO_ALIVE = false;
				}
				
				var curID:int;
				
				aliveBlobs.endian = Endian.LITTLE_ENDIAN;
				
				while(aliveBlobs.bytesAvailable>0){
					curID = aliveBlobs.readInt();
					var curBlobObject = getObjectById(curID);
					if(DEBUG && DEBUGGER.on && curBlobObject!=null) DEBUGGER.addDebugText("Blob #"+curID+" ("+int(curBlobObject.x)+", "+int(curBlobObject.y)+")\n");
					if(getObjectById(curID)){
						getObjectById(curID).TUIO_ALIVE = true;
					}
				}
				
				var aliveObjLen:int = OBJECT_ARRAY.length;
				
				for(var i:int=0; i<aliveObjLen; i++){
					if(OBJECT_ARRAY[i].TUIO_ALIVE == false){
						OBJECT_ARRAY[i].notifyRemoved();
						STAGE.removeChild(OBJECT_ARRAY[i].TUIO_CURSOR);
						OBJECT_ARRAY.splice(i, 1);
						i--;
						aliveObjLen = OBJECT_ARRAY.length;
					}
				}
				aliveMessage = false;
			}
						
			byteArray = null;			
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
        private static function activateDebugMode():void{
  			DEBUGGER = new TUIODebugger();
			STAGE.addChild(DEBUGGER);
			DEBUGGER.x = 10
			DEBUGGER.y = 10;
			DEBUGGER.on = true;
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
		private static function socketStatus(e:Event):void{ 		
			SOCKET_STATUS = SOCKET.connected;
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
        private static function dataHandler(e:ProgressEvent):void{    
			var dump:ByteArray = new ByteArray(),
			    startTime:int = getTimer();
			
			SOCKET.readBytes(dump);
			
			processMessage(dump);
			
			var finishTime:int = getTimer();
			
			if(DEBUG && DEBUGGER.on){
				DEBUGGER.clearFooterText();
				DEBUGGER.addFooterText("Time test - " + (finishTime - startTime) + " ms");
			}
			
			dump = null;
        } 
//---------------------------------------------------------------------------------------------------------------------------------------------   			
		private static function closeHandler(event:Event):void{
			startSocket();
			socketStatus(null);
        }
     	private static function connectHandler(event:Event):void{	
			socketStatus(null);			
        }               
        private static function ioErrorHandler(event:IOErrorEvent):void{
			startSocket();
        }
        private static function securityErrorHandler(event:SecurityErrorEvent):void{
			startSocket();
        }
//---------------------------------------------------------------------------------------------------------------------------------------------
		/*private static function byteArrayBeginsWithString(byteArray:ByteArray,searchString:String,byteLength:Number = -1):Boolean{ //speed increase when bytelength supplied??
			if(byteLength<0) byteLength = searchString.length;
			if(byteLength>byteArray.bytesAvailable) return false;
			if(byteArray.readUTFBytes(byteLength)===searchString){
				return true;
			}else{
				byteArray.position -= byteLength;
				return false;
			}
		}*/

	}
}