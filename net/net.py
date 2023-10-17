#cording utf-8

from re import sub
import requests
import sys
import json
import os
import urllib3
from urllib3.exceptions import InsecureRequestWarning

import cat
urllib3.disable_warnings(InsecureRequestWarning)

# 自作モジュール
import cat


URL = "https://procon33-practice.kosen.work"
TOKEN = "<ここにトークンを入力してください>"
query = "?token=" + TOKEN


"""エラー一覧"""
err = ["InvalidToken", "AccessTimeError", "FormatError", "NotFound", "TooLargeRequestError", "Gateway Timeout"]


"""予期せぬ結果が出たときに例外処理する関数"""
def alert(j):
    if j is None:
        print("None!!")
        sys.exit()
    elif (j.text[0:6] == "<html>"):
        print("it is html!!")
        sys.exit()
    for i in range(len(err)):
        if err[i] in j.text:
            print(j.text)
            sys.exit()



"""jsonファイルを作成する"""
def make_json(file_name, text):
    # 受け取った文字列を辞書型に変換
    dictionary = json.loads(text)
    with open(file_name, "w") as j:
        json.dump(dictionary, j, indent=2)




"""/matchにGETリクエストを送って、試合情報のjsonを返す"""
def match_get():
    res_match = requests.get(URL + "/match" + query, verify=False)
    alert(res_match)
    print(res_match.text)
    make_json("match.json", res_match.text)
    return res_match.text


"""/problemにGETリクエストを送って、問題情報のjsonを返す"""
def problem_get():
    res_problem = requests.get(URL + "/problem" + query, verify=False)
    alert(res_problem)
    make_json("problem.json", res_problem.text)
    # 受けとったjsonを辞書型に変換
    problem_dic = json.loads(res_problem.text)
    # 札数を../analyze/testのinformation.txtに書き込む
    with open("../analyze/test/information.txt", "w") as txt:
        print(problem_dic["data"], file=txt)
    # 問題IDをproblem.txtに書き込む
    with open("problem.txt", "w") as txt:
        print(problem_dic["id"], file=txt)
    # 標準出力で取得した問題の制限時間を出力する
    print("制限時間は" + str(problem_dic["time_limit"]) + "秒")
    # 変換したjsonから選択数を抽出する
    return problem_dic


"""/problem/chunksに分割数をPOSTリクエストで送って、各分割データのファイル名の配列のjsonを返す"""
def chunks_post(select):
    res_chunks = requests.post(URL + "/problem/chunks" + query + "&n=" + str(select), verify=False)
    alert(res_chunks)
    make_json("chunks.json", res_chunks.text)
    return res_chunks.text


"""/problem/chunks/:filenameにGETリクエストを送って、分割データのwavファイルを返す"""
def file_get(res_chunks):
    # ファイル名配列のjsonを辞書型に変換する
    filenames = json.loads(res_chunks)
    # audioディレクトリに移動、もしなければaudioディレクトリを作成し移動する
    try:
        os.chdir("audio")
    except:
        os.mkdir("audio")
        os.chdir("audio")
    # ファイル名配列の長さ分だけfor分を回し、それぞれに対応するwavファイルをダウンロードする
    for i in range(len(filenames["chunks"])):
        # ファイル名配列のi番目にあるファイルにGETリクエストを送る
        res_wav = requests.get(URL + "/problem/chunks/" + (filenames["chunks"])[i] + query, verify=False)
        # audioディレクトリにwavファイルをダウンロード
        with open((filenames["chunks"])[i], "wb") as saveFile:
            saveFile.write(res_wav.content)
    # ディレクトリを元に戻す
    os.chdir("..")





"""../analyzeにあるres.txtから札の種類を読み込む"""
def read_res():
    res_ls = []
    # res.txtに書かれている札種を半角スペース区切りで分割してリストにする
    with open("../analyze/res.txt") as txt:
        res = txt.read()
        res_ls = (res.replace("\n", "")).split(" ")
    # 読み込んだリストを整数型にすると同時に提出用に数字を編集する
    for i in range(len(res_ls)):
        res_ls[i] = (int(res_ls[i])) % 44 + 1
    # res.txtから読み込んだリストを回答形式に合うように変更してリストにす る
    res_ls.sort()
    fuda_ls = []
    for fuda in res_ls:
        if len(str(fuda)) == 1:
            fuda_ls.append("0" + str(fuda))
        else:
            fuda_ls.append(str(fuda))
    print(fuda_ls)
    return fuda_ls


"""提出用のjsonファイルを生成"""
def mkjson(fuda_ls):
    # problem.txtから問題IDを読み込み
    problem_id = ""
    with open("problem.txt", "r") as txt:
        problem_id = (txt.read()).replace("\n", "")
    # 辞書型作成
    dic = {"problem_id":problem_id, "answers":fuda_ls}
    # jsonファイルへ書き込み
    with open("submit.json", "w") as j:
        json.dump(dic, j, indent=2)



"""/problemに回答のjsonをPOSTリクエストで送り、返ってきたjsonファイルを受け取る"""
def problem_post():
    with open("submit.json", "r") as submit_json:
        res_problem = requests.post(URL + "/problem" + query,
                                    json=json.load(submit_json),
                                    headers={'Content-Type': 'application/json'},
                                    verify=False)
    alert(res_problem)
    make_json("response.json", res_problem.text)


if __name__ == "__main__":
    """引数の数に過不足があったら終了させる"""
    if len(sys.argv) != 2:
        sys.exit()
        
    """最終的に音声ファイルをダウンロードして結合"""
    if (sys.argv[1] == "download" or sys.argv[1] == "Download" or sys.argv[1] == "DOWNLOAD"):
        match_get()
        problem_information = problem_get()
        file_get(chunks_post(problem_information["chunks"]))
        cat.wav_cat()
    """分析結果のjsonファイルを提出する"""
    if (sys.argv[1] == "submit" or sys.argv[1] == "Submit" or sys.argv[1] == "SUBMIT"):
        mkjson(read_res())
        problem_post()
