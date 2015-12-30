// Deobfuscator

#include "stdafx.h"
#include "pe.h"
#include "trace.h"
#include <iostream>

int main(int argc, char** argv)
{
	// TODO: use cmd_line option, and parse it

	std::string original_pe_path
		= "C:\\Users\\ogamal\\Documents\\binary_code_obfuscation_sample\\bin\\overlap_func.exe";
	std::string trace_file_path
		= "C:\\Users\\ogamal\\Documents\\trace_data\\overlapping_functions.log";
	std::string out_put_binary_path
		= "C:\\Users\\ogamal\\Documents\\hoge_overlap.exe";

	// Read
	PEReader::Builder builder;
	PEReader *reader = builder.setInputFilePath(original_pe_path)->build();
	if (reader == nullptr) {
		std::cout << builder.getErrMsg() << std::endl;
		exit(1);
	}
	PEFormat pe_fmt = reader->getPEFormat();

	// Analyze
	TraceReader trace_reader;
	trace_reader.openTraceFile(trace_file_path);
	std::unique_ptr<TraceData> data = trace_reader.read();
	TraceAnalyzer analyzer(std::move(data));
	std::unique_ptr<TraceAnalysisResult> analysis_result = analyzer.analyze();

	// Edit
	PEEditor editor(pe_fmt, std::move(analysis_result));
	PEFormat *modified_pe = editor.result();

	// Write
	PEWriter::Builder writer_builder;
	PEWriter *writer = writer_builder
		.setOutputFilePath(out_put_binary_path)
		->setPEFormat(modified_pe)
		->build();
	if (writer == nullptr) {
		std::cout << writer_builder.getErrMsg() << std::endl;
		exit(1);
	}
	writer->write();
	return 0;
}