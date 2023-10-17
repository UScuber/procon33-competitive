import soundfile as sf
import numpy as np
import glob
import os

def wav_cat():
    # 現在のディレクトリ
    cur_dir = os.getcwd()

    # audioディレクトリに入っているwavファイル名(パス)をリスト形式で取得
    file_names = glob.glob("audio/*.wav")

    sound = []

    # 取得したwavファイル名(パス)を順に結合していく
    for file_name in file_names:
        audio, fs = sf.read(file_name)
        sound = np.concatenate([sound, audio], 0)

    # analyze/testディレクトリの方に移って結合したwavファイルを出力する
    os.chdir("../analyze/test")
    sf.write("problem.wav", sound, fs)

    # 現在のディレクトリに移って受け取った分割データを全削除
    os.chdir(cur_dir)
    for file_name in file_names:
        os.remove(file_name)

if __name__ == "__main__":
    wav_cat()