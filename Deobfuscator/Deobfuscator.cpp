// Deobfuscator.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "pe.h"
#include "trace.h"
#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
	if (argc != 2) {
		cout << "invalid arguments" << endl;
		return -1;
	}
	// Read
	PEReader::Builder builder;
	PEReader *reader = builder.setInputFilePath(argv[1])->build();
	if (reader == nullptr) {
		cout << builder.getErrMsg() << endl;
		exit(1);
	}
	PEFormat pe_fmt = reader->getPEFormat();

	// Analyze
	TraceReader trace_reader;
	trace_reader.openTraceFile("C:\\Users\\ogamal\\Documents\\trace_data\\overlapping_functions.log");
	std::shared_ptr<TraceData> data = trace_reader.read();
	TraceAnalyzer analyzer(*data);
	std::unique_ptr<TraceAnalysisResult> analysis_result = analyzer.analyze();

	// Edit
	PEEditor editor(pe_fmt, std::move(analysis_result));
	PEFormat *modified_pe = editor.result();

	// Create new section
	/*
	std::vector<char> new_code({ (char)(unsigned char)0xC3 });
	PEEditor::SectionHeaderBuilder b = PEEditor::SectionHeaderBuilder();
	b.setName((BYTE *)string(".shadow").c_str(), 7)
		->setPointerToRawData(0xC00)
		->setSizeOfRawData(0x200)
		->setVirtualAddress(0x4000)
		->setVirtualSize(0x1)
		->setCharacteristcs(0x60000020);
	IMAGE_SECTION_HEADER shadow_section_header = b.build();

	editor.addSection(shadow_section_header, new_code);
	PEFormat *modified_pe = editor.result();
	*/

	// Write
	PEWriter::Builder writer_builder;
	PEWriter *writer = writer_builder
		.setOutputFilePath("\\Users\\ogamal\\Documents\\hoge_overlap.exe")
		->setPEFormat(modified_pe)
		->build();
	if (writer == nullptr) {
		cout << writer_builder.getErrMsg() << endl;
		exit(1);
	}
	writer->write();
	return 0;
}