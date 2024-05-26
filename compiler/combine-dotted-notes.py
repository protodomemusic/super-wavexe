################################################################################
# A quick tool to remove redundant note pairings. For example, c4^c8 could just
# be c4. - saves some bytes. This is eventually going to be part of the 4ML
# cleanup/compress functions, but for now it can just go in here.
################################################################################

import re

def transform_notes(text, original_note, target_note):	

	# input defs
	# - text: the input string containing note patterns
	# - original_note: the original note value to be transformed
	# - target_note: the target note value that replaces the original

	pattern     = rf'([a-zA-Z]){original_note}\^\1(?={target_note})'
	replacement = rf'\1 {original_note}.'
	text        = re.sub(pattern, replacement, text)
	text        = text.replace(f' {original_note}.{target_note}', f'{original_note}.')
	return text

def process_text(text, transformations):

	# input defs:
	# text: the input text to process
	# transformations: a list of tuples, each containing the original and target note values

	for original_note, target_note in transformations:
		text = transform_notes(text, original_note, target_note)
	return text

# configuration of note transformations
transformations = [
	('2', '2'),   # dotted half note
	('4', '8'),   # dotted quarter note
	('8', '16'),  # dotted eighth note
	('16', '32'), # dotted sixteenth note
	('32', '64'), # dotted thirty-second note
	('64', '128') # dotted sixty-fourth note
]

# read the input file
input_file_path = 'input.4ml'
with open(input_file_path, 'r') as file:
	input_text = file.read()

# process the text
processed_text = process_text(input_text, transformations)

# write the processed text to an output file
output_file_path = 'output.4ml'
with open(output_file_path, 'w') as file:
	file.write(processed_text)

print("Processing complete. Output written to:", output_file_path)
