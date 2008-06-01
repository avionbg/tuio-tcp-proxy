package flash.events{
	import flash.display.Sprite;
	import flash.events.MouseEvent;
	import gs.TweenLite;
	import flash.text.TextField;
	import flash.text.TextFormat;
	import flash.geom.Matrix;
	import flash.events.TouchEvent;
	import flash.events.MouseEvent;
	
	public class TUIODebugger extends Sprite{
		private const WIDTH:Number = 150;
		private const HEIGHT:Number = 40;
		private const PADDING:Number = 8;
		private var _button:Sprite;
		private var _title:TextField;
		private var _statusDisplay:Sprite;
		private var _debugText:TextField;
		private var _debugTextBg:Sprite;
		private var _on:Boolean;
		private var _cachedDubugText:String;
		private var _cachedFooterText:String;
		
		public function TUIODebugger(titleStr:String = "touchlib: debug"):void{
			_button = new Sprite();
			_button.graphics.beginFill(0x111111);
			_button.graphics.lineStyle(2,0x666666,.5,true);
			_button.graphics.drawRoundRect(0,0,WIDTH,HEIGHT,20);
			_button.graphics.endFill();
			
			var format:TextFormat = new TextFormat("Arial", 13, 0xFFFFFF,true);
			_title = new TextField();       
			_title.defaultTextFormat = format;
			_title.autoSize = "left";
			_title.selectable = false;
			_title.text = titleStr;
			
			format.bold = false;
			format.size = 12;
			
			_debugText = new TextField();
			_debugText.defaultTextFormat = format;
			_debugText.selectable = false;
			_debugText.autoSize = "left";
			_debugText.multiline = true;
			_debugText.width = WIDTH-(PADDING*2);
						
			_debugTextBg = new Sprite();
			
			_statusDisplay = new Sprite();
			_statusDisplay.graphics.lineStyle(1,0x455567,.75);
			_statusDisplay.graphics.beginFill(0x97f324,1);
			_statusDisplay.graphics.drawCircle(0,0,HEIGHT*.15);
			_statusDisplay.graphics.endFill();
			
			var sdb:Sprite = new Sprite();
			sdb.graphics.lineStyle(1,0x455567,.75);
			sdb.graphics.beginFill(0x000000);
			sdb.graphics.drawCircle(0,0,HEIGHT*.15);
			sdb.graphics.endFill();
			
			addChild(_debugTextBg);
			addChild(_debugText);
			addChild(_button);
			addChild(_title);
			addChild(sdb);
			addChild(_statusDisplay);

			_title.y = (_button.height/2)-(_title.height/2);
			_title.x = PADDING;
			_debugText.y = HEIGHT+PADDING;
			_debugText.x = PADDING;
			_statusDisplay.y = (_button.height/2);
			_statusDisplay.x = WIDTH-PADDING-(_statusDisplay.width/2);
			sdb.x=_statusDisplay.x;
			sdb.y=_statusDisplay.y;
			
			_button.addEventListener(MouseEvent.CLICK, toggleDebug);
			_button.addEventListener(TouchEvent.CLICK, toggleDebug);
			_title.addEventListener(MouseEvent.CLICK, toggleDebug);
			_title.addEventListener(TouchEvent.CLICK, toggleDebug);
			_statusDisplay.addEventListener(MouseEvent.CLICK, toggleDebug);
			_statusDisplay.addEventListener(TouchEvent.CLICK, toggleDebug);
			sdb.addEventListener(MouseEvent.CLICK, toggleDebug);
			sdb.addEventListener(TouchEvent.CLICK, toggleDebug);
			
			_cachedDubugText="";
			_cachedFooterText="";
			
			debugOff();
			resizeDebugTextBg();
		}
		
		private function toggleDebug(evnt:*=null):void{
			on = !on;
		}
		
		private function debugOff():void{
			_debugText.visible = false;
			_statusDisplay.visible = false;
			//_debugTextBg.visible = false;
			_debugText.text="";
			resizeDebugTextBg();
			_on = false;
		}
		
		private function debugOn():void{
			_debugText.visible = true;
			_statusDisplay.visible = true;
			//_debugTextBg.visible = true;
			_on = true;
		}
		
		public function clearDebugText():void{
			_cachedDubugText="";
			constructText();
		}
		
		public function addDebugText(s:String):void{
			_cachedDubugText+=s;
			constructText();
		}
		
		
		public function clearFooterText():void{
			_cachedFooterText="";
			constructText();
		}
		
		public function addFooterText(s:String):void{
			_cachedFooterText+=s;
			constructText();
		}
		
		private function constructText():void{
			var concat:String = _cachedDubugText+(_cachedDubugText.length != 0 ? "\n\n" : "")+(_cachedFooterText.length == 0 ? "" : _cachedFooterText);
			
			if(concat == _debugText.text) return;
			
			_debugText.text = concat;
			
			resizeDebugTextBg();
		}
		
		private function resizeDebugTextBg():void{
			_debugTextBg.graphics.clear();
			_debugTextBg.graphics.beginFill(0x000000,.3);
			_debugTextBg.graphics.lineStyle(2,0xFFFFFF,.1);
			if(_debugText.text.length==0){
				_debugTextBg.graphics.drawRoundRect(0,0,WIDTH,HEIGHT,20);
			}else{
				_debugTextBg.graphics.drawRoundRect(0,0,WIDTH,HEIGHT+(PADDING*2)+_debugText.height,20);
			}
			_debugTextBg.graphics.endFill();
		}
		
		public function get on():Boolean{
			return _on;
		}

		public function set on(b:Boolean):void{
			if(b==true){
				debugOn();
			}else{
				debugOff();
			}
		}
	}
}