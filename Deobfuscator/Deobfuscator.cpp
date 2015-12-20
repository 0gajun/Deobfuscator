// Deobfuscator.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "pe.h"
#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
	if (argc != 2) {
		cout << "invalid arguments" << endl;
		return -1;
	}

	PEReader::Builder builder;
	PEReader *reader = builder.setInputFilePath(argv[1])->build();
	if (reader == nullptr) {
		cout << builder.getErrMsg() << endl;
		exit(1);
	}

	PEFormat pe_fmt = reader->getPEFormat();

	PEWriter::Builder writer_builder;
	PEWriter *writer = writer_builder
		.setOutputFilePath("\\Users\\ogamal\\Documents\\hoge.exe")
		->setPEFormat(&pe_fmt)
		->build();
	if (writer == nullptr) {
		cout << writer_builder.getErrMsg() << endl;
		exit(1);
	}
	writer->write();
	return 0;
}