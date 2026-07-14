// Spetrum View for FMRaduino
// by Musashinodenpa 2009

import processing.serial.*;
Serial serial;
int lf = 10;

void setup() {
  // シリアルポートは自分の環境に合わせて選択してください
  serial = new Serial(this, "COM8", 9600);

  size(600, 240);
  background(#000040);
  rectMode(CORNERS);
  stroke(#404080);

  // 罫線の描画
  for(int i=0; i<8; i++) {
    line(40, 190-20*i, 570, 190-20*i);
  }
  for(int i=700; i<901; i=i+100) {
    line((i-690)*2+50, 190, (i-690)*2+50, 50);
  }
  stroke(#a0a020);

}

void draw() {
  String str = null;

  // Freq = Valueの形式でデータが送られてくるので
  // それをもとに棒グラフを描画
  while (serial.available() > 0) {
    str = serial.readStringUntil(lf);
    if (str != null) {

      String v[] = split(str, " = ");
      int x = int(v[0]) - 690;
      int y = int(v[1].trim());
      rect(x*2+50, 190, x*2+51, 190-y);
      redraw();
    }
  }
}
