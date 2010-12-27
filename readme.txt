DXOpenNI.dll is a driver to read the position of the joint from Kinect.
Person's depth information can be drawn as a texture of DirectX. 

It is also possible that this driver uses as a plug-in of MikuMikuDance. 
Before using this with MikuMikuDance, You must install OpenNI to your PC
and get in the state that the sample of OpenNI can be moved. 
Then please copy two files "DxOpenNI.dll, SamplesConfig.xml" to "Data"folder
of MikuMikuDance.

This driver is using the OpenNI driver. It is open to the public in the GNU
LGPL license. Therefore, I will open DXOpenNI.dll source code to the public. 
(The code of OpenNI is not included in MikuMikuDance at all. )
See the GNU General Public License for more details:
<http://www.gnu.org/licenses/>.


--Japanese--
DXOpenNI.dllはKinectから関節の位置を読み取るためのドライバです。
人物の深度情報をDirectXのテクスチャとして描き出すことができます。

このドライバはMikuMikuDanceのプラグインとして使用することも可能です。
MikuMikuDanceで使用する場合は、OpenNIをインストールしてOpenNIの
サンプルを動かせる状態にしたうえで、"DxOpenNI.dll"と"SamplesConfig.xml"の
２つのファイルをMikuMikuDanceの"Data"フォルダ内にコピーして下さい。

このドライバはOpenNIドライバを使用しています。OpenNIは
GNU LGPLライセンスで公開されています。ですので、このドライバのソース
コードも公開いたします。
(MikuMikuDanceにはOpenNIのコードは一切含まれておりません)
GNU LGPLライセンスについて詳しく知りたい方は以下のHPをご覧下さい。
<http://www.gnu.org/licenses/>.