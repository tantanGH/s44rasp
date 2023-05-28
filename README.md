# s44rasp

ADPCM/PCM/WAV player for Raspberry Pi

---

## About This

---

## How to Install


---

## USB DAC の利用

ラズパイ標準のヘッドフォンジャックからの音声出力は簡易的なPWM出力なのでとても音質が悪く、音楽を聴くための設計にはなっていません。

もし手元にPC用の USB DAC、あるいはスマホ用の USB - ヘッドフォンケーブルなどがあれば、それをラズパイに挿して、D/A変換を USB DAC に委ねることで音質の劇的な改善が期待できます。

自分は ONKYO の DAC-HA200 を使っています(既に廃盤)。

[ONKYO DAC-HA200](https://www.jp.onkyo.com/audiovisual/headphone/dacha200/)

音質もさることながら、ボリュームノブの操作感が良く、あまりに気に入ったので2台持ってますw

s44rasp でこの種の USB-DAC を利用するには、USB端子にDACを接続した後、`aplay -l`で確認します。

        $ aplay -l

        **** List of PLAYBACK Hardware Devices ****
        card 0: Headphones [bcm2835 Headphones], device 0: bcm2835 Headphones [bcm2835 Headphones]
          Subdevices: 8/8
          Subdevice #0: subdevice #0
          Subdevice #1: subdevice #1
          Subdevice #2: subdevice #2
          Subdevice #3: subdevice #3
          Subdevice #4: subdevice #4
          Subdevice #5: subdevice #5
          Subdevice #6: subdevice #6
          Subdevice #7: subdevice #7
        card 1: DACHA200 [DAC-HA200], device 0: USB Audio [USB Audio]
          Subdevices: 1/1
          Subdevice #0: subdevice #0
        card 2: vc4hdmi [vc4-hdmi], device 0: MAI PCM i2s-hifi-0 [MAI PCM i2s-hifi-0]
          Subdevices: 1/1
          Subdevice #0: subdevice #0

この場合は card 1 として認識されていますので、s44rasp で利用する場合は、以下のように指定すればok。

        s44rasp -d hw:1,0 hogehoge.s44

---

## Hi-Fi DAC HAT の利用

Raspberry Pi の 40pin GPIO コネクタにポン付けできる DAC HAT が数多く市販されています。特にバーブラウン社製のPCM51xxチップを使ったものは定番です。
数千円の出費でラズパイの出力音質を劇的に向上させることが可能な上、ラズパイと一体化できて場所もとらずお勧めです。自分は InnoMaker 製のものを使っています。

[InnoMaker Hi-Fi DAC CAP](https://www.amazon.co.jp/dp/B07TFHNPCB/)

これを ALSA から利用するためには Raspberry Pi OS の`/boot/config.txt`を次のように変更する必要があります。

        sudo vi /boot/config.txt

以下の行のコメントを外し、I2Sを有効にする。

        dtparam=i2s=on

以下の行をコメントアウトし、デフォルトのPWMオーディオ出力を無効にする。

        #dtparam=audio=on

以下の行を追加し、PCM51xx用のドライバを追加する。

        dtoverlay=hifiberry-dac

再起動し、`aplay -l`で DAC HAT が認識されていることを確認する。

        $ aplay -l

        **** List of PLAYBACK Hardware Devices ****
        card 0: sndrpihifiberry [snd_rpi_hifiberry_dac], device 0: HifiBerry DAC HiFi pcm5102a-hifi-0 [HifiBerry DAC HiFi pcm5102a-hifi-0]
          Subdevices: 0/1
          Subdevice #0: subdevice #0
        card 1: DACHA200 [DAC-HA200], device 0: USB Audio [USB Audio]
          Subdevices: 1/1
          Subdevice #0: subdevice #0
        card 2: vc4hdmi [vc4-hdmi], device 0: MAI PCM i2s-hifi-0 [MAI PCM i2s-hifi-0]
          Subdevices: 1/1
          Subdevice #0: subdevice #0        

この場合 card 0 として認識されているので、s44rasp で利用する場合は、以下のように指定すればok。

        s44rasp -d hw:0,0 hogehoge.s44

もしくは特にdefaultの出力先をALSAに登録していなければ、指定なしでもok

        s44rasp hogehoge.s44

---

## OLED (SSD1306) の利用

