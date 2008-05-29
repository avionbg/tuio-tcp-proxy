package flash.events{
	import flash.display.Sprite;
	import flash.events.MouseEvent;
	import gs.TweenLite;
	import flash.text.TextField;
	import flash.text.TextFormat;
	import flash.geom.Matrix;
	
	public class TUIODebugger extends Sprite{
		private const WIDTH:Number = 150;
		private const HEIGHT:Number = 40;
		private const PADDING:Number = 8;
		private var _button:Sprite;
		private var _title:TextField;
		private var _statusDisplay:Sprite;
		private var _debugText:TextField;
		private var _debugTextBg:Sprite;;
		
		public function TUIODebugger(titleStr:String = "touchlib: debug"):void{
			var bMatrix:Matrix = new Matrix();
			bMatrix.createGradientBox(WIDTH,HEIGHT,90*Math.PI/180)
			_button = new Sprite();
			_button.graphics.beginGradientFill("linear",[0x2c3239,0x151618],[1,1],[0,255],bMatrix);
			_button.graphics.lineStyle(1,0x455567,.75);
			_button.graphics.drawRoundRect(0,0,WIDTH,HEIGHT,15);
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
			//_debugText.text="Blob ID: 1249";
			_debugText.width = WIDTH-(PADDING*2);
						
			_debugTextBg = new Sprite();
						
			/*_switchBgOn = new Sprite();
			_switchBgOn.graphics.beginFill(0x97f324);
			_switchBgOn.graphics.drawRoundRect(0,0,40,HEIGHT*.4,6);
			*/
			
			_statusDisplay = new Sprite();
			_statusDisplay.graphics.beginFill(0x97f324);
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
			//addChild(_switchBgOn);
			addChild(_title);
			addChild(sdb);
			addChild(_statusDisplay);
			
			//_switchBgOn.y = (_button.height/2)-(_switchBgOn.height/2);
			//_switchBgOn.x = WIDTH-PADDING-_switchBgOn.width;
			
			_title.y = (_button.height/2)-(_title.height/2);
			_title.x = PADDING;
			_debugText.y = HEIGHT+PADDING;
			_debugText.x = PADDING;
			_statusDisplay.y = (_button.height/2);
			_statusDisplay.x = WIDTH-PADDING-(_statusDisplay.width/2);
			sdb.x=_statusDisplay.x;
			sdb.y=_statusDisplay.y;
			off();
			resizeDebugTextBg();
		}
		
		public function off():void{
			_debugText.visible = false;
			_statusDisplay.visible = false;
			_debugTextBg.visible = false;
		}
		
		public function on():void{
			_debugText.visible = true;
			_statusDisplay.visible = true;
			_debugTextBg.visible = true;
		}
		
		public function clearDebugText():void{
			_debugText.text="";
			resizeDebugTextBg();
		}
		
		public function addDebugText(s:String):void{
			_debugText.appendText(s);
			resizeDebugTextBg();
		}
		
		private function resizeDebugTextBg():void{
			_debugTextBg.graphics.clear();
			if(_debugText.text.length==0) return;
			_debugTextBg.graphics.beginFill(0x2c3239,.7);
			_debugTextBg.graphics.drawRoundRect(0,0,WIDTH,HEIGHT+(PADDING*2)+_debugText.height,15);
			_debugTextBg.graphics.endFill();
		}
	}
}