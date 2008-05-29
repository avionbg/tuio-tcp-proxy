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
		private static var FRAME_COUNT:Number;
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
		public static function init ($STAGE:DisplayObjectContainer, $HOST:String, $PORT:Number, $DEBUG:Boolean = true):void{
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
		public static function addObjectListener(id:Number, receiver:Object):void{
			var tmpObj:TUIOObject = getObjectById(id);			
			if(tmpObj){
				tmpObj.addListener(receiver);				
			}
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
		public static function removeObjectListener(id:Number, receiver:Object):void{
			var tmpObj:TUIOObject = getObjectById(id);			
			if(tmpObj){
				tmpObj.removeListener(receiver);				
			}
		}		
//---------------------------------------------------------------------------------------------------------------------------------------------
		public static function getObjectById(id:Number):TUIOObject{
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
			
			var time = getTimer();
			var update:Boolean;
										
			while(byteArray.bytesAvailable>0){
				if(byteArrayBeginsWithString(byteArray,"/tuio/2Dcur")){
					byteArray.position++;
					
					if(byteArrayBeginsWithString(byteArray,"alive")){
						update = true;
						
						if(DEBUG){
							DEBUGGER.clearDebugText();
						}
						
						byteArray.position+=3;
						
						var numBlobs = byteArray.readInt();
												
						for each (var obj1:TUIOObject in OBJECT_ARRAY){
							obj1.TUIO_ALIVE = false;
						}
						
						var curID:int;
						
						for(var h=0;h<numBlobs-1;h++){
							curID = byteArray.readInt();
							if(DEBUG){
								DEBUGGER.addDebugText("Blob id: "+curID+"\n");
							}
							if(getObjectById(curID)){
								getObjectById(curID).TUIO_ALIVE = true;
							}
						}
						
						
						for (var i:int=0; i<OBJECT_ARRAY.length; i++ ){	
							if(OBJECT_ARRAY[i].TUIO_ALIVE == false){
								OBJECT_ARRAY[i].notifyRemoved();
								STAGE.removeChild(OBJECT_ARRAY[i].TUIO_CURSOR);
								OBJECT_ARRAY.splice(i, 1);
								i--;
							}
						}
						
					}else if(byteArrayBeginsWithString(byteArray,"set")){					
						
						byteArray.position+=5;				
															
						if(byteArray.bytesAvailable>=(7*4)){
							var id:int = byteArray.readInt();
							var x:Number = byteArray.readFloat()* STAGE.stageWidth;
							var y:Number = byteArray.readFloat()* STAGE.stageHeight;
							var X:Number = byteArray.readFloat();
							var Y:Number = byteArray.readFloat();
							var m:Number = byteArray.readFloat();
							var wd:Number = byteArray.readFloat();
							var ht:Number = byteArray.readFloat();;
							
							var stagePoint:Point = new Point(x,y);					
							var displayObjArray:Array = STAGE.getObjectsUnderPoint(stagePoint);
										
							var dobj:DisplayObject = null;
											
							if(displayObjArray.length > 0){
								dobj = displayObjArray[displayObjArray.length-1];	
							}
																								
							var tuioobj:TUIOObject = getObjectById(id);
							if(tuioobj == null){
								if(id==0) trace("zero blob")
								tuioobj = new TUIOObject("2Dcur", id, x, y, X, Y, -1, 0, wd, ht, dobj);
								STAGE.addChild(tuioobj.TUIO_CURSOR);								
								OBJECT_ARRAY.push(tuioobj);
								tuioobj.notifyCreated();
							}else {
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
								if(!( int(Y*1000) == 0 && int(Y*1000) == 0)){
									tuioobj.notifyMoved();
								}
								
								if( int(Y*250) == 0 && int(Y*250) == 0) {
									var lastModDur:Number = d.time - tuioobj.lastModifiedTime;
									if(Math.abs(lastModDur) > HOLD_THRESHOLD){
										for(var ndx:int=0; ndx<EVENT_ARRAY.length; ndx++){
											EVENT_ARRAY[ndx].dispatchEvent(tuioobj.getTouchEvent(TouchEvent.LONG_PRESS));
										}
										tuioobj.lastModifiedTime = d.time;																		
									}
									
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
					}else if(byteArrayBeginsWithString(byteArray,"fseq")){
						
						byteArray.position+=4;
						
						FRAME_COUNT = byteArray.readInt();
						//trace("[FSEQ]",FRAME_COUNT);
					}
				}else{
					if(byteArray.bytesAvailable>0) byteArray.position++; //only change if nothing found
				}
			}

			var finalTime = getTimer();
			if(DEBUG && update){
				DEBUGGER.addDebugText("\nTime test - " + (finalTime - time) + " ms");
				update = false;
			}
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
        private static function activateDebugMode():void{
  				DEBUGGER = new TUIODebugger();
				STAGE.addChild(DEBUGGER);
				DEBUGGER.addEventListener(MouseEvent.CLICK, toggleDebug);		
				DEBUGGER.addEventListener(TouchEvent.CLICK, toggleDebug);	
				DEBUGGER.x=10
				DEBUGGER.y=10;
				DEBUGGER.on();
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
		private static function socketStatus(e:Event):void{ 		
				SOCKET_STATUS = SOCKET.connected;
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
		private static function toggleDebug(e:Event):void{ 			
			if(!DEBUG){
				DEBUG=true;	
				DEBUGGER.on();
			}else{
				DEBUG=false;
				DEBUGGER.off();
			}	
		}
//---------------------------------------------------------------------------------------------------------------------------------------------
        private static function dataHandler(e:ProgressEvent):void{    
			var dump = new ByteArray();
			SOCKET.readBytes(dump);
			SOCKET.flush();
			
			processMessage(dump);
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
		private static function byteArrayBeginsWithString(byteArray:ByteArray,searchString:String):Boolean{
			var strLen:Number = searchString.length;
			if(strLen>byteArray.bytesAvailable) return false;
			var checkByteString:String = byteArray.readUTFBytes(strLen)			
			if(checkByteString==searchString){
				return true;
			}else{
				byteArray.position -= strLen;
				return false;
			}
		}

	}
}