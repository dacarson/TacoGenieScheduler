/*
 Copyright (c) 2024 David Carson (dacarson)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

const days = ['monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday', 'sunday'];

// Initialize schedule containers for each day
days.forEach(day => {
	const container = document.createElement('div');
	container.classList.add('day-container');
	container.id = day;
	container.innerHTML = `
   <h2>${day.charAt(0).toUpperCase() + day.slice(1)}</h2>
   <div class="time-slots" id="${day}-slots"></div>
   <button onclick="addTimeSlot('${day}')">Add Time Slot</button>
   <button onclick="copySchedule('${day}')" data-action='copy'>Copy Schedule To...</button>`;
	document.getElementById('schedule').appendChild(container);
	updateCopyButtonVisibility(day);
});

function loadScheduleFromJSON(scheduleJSON) {
	const schedule = JSON.parse(scheduleJSON);
	for (const day in schedule) {
		const slotsContainer = document.getElementById(day + '-slots');
		slotsContainer.innerHTML = ''; // Clear existing slots
		schedule[day].forEach(slot => {
			const div = document.createElement('div');
			div.classList.add('time-slot');
			div.innerHTML = `
	   <input type="time" class="start-time" value="${slot.start}" onchange="validateTimeSlot(this); updateCopyButtonVisibility('${day}')" /> to
	   <input type="time" class="end-time" value="${slot.end}" onchange="validateTimeSlot(this); updateCopyButtonVisibility('${day}')" />
	   <button class="remove-button" onclick="removeTimeSlot(this, '${day}')">❌</button>`;
			slotsContainer.appendChild(div);
			updateCopyButtonVisibility(day);
		});
	}
}

function removeTimeSlot(button, day) {
	button.parentNode.remove();
	updateCopyButtonVisibility(day);
}

function addTimeSlot(day) {
	const container = document.getElementById(day + '-slots');
	const existingEntries = container.getElementsByClassName('time-slot').length;

    if (existingEntries >= 10) {
        alert('You can only create up to 10 start/end times per day.');
        return; // Stop the function from adding another time slot
    }
    
	const div = document.createElement('div');
	div.classList.add('time-slot');
	div.innerHTML = `
   <input type="time" class="start-time" onchange="validateTimeSlot(this); updateCopyButtonVisibility('${day}')" /> to
   <input type="time" class="end-time" onchange="validateTimeSlot(this); updateCopyButtonVisibility('${day}')" />
   <button class="remove-button" onclick="removeTimeSlot(this, '${day}')">❌</button>`;
	container.appendChild(div);
	updateCopyButtonVisibility(day);
}

function validateTimeSlot(input) {
	const timeSlot = input.closest('.time-slot');
	const startTimeInput = timeSlot.querySelector('.start-time');
	const endTimeInput = timeSlot.querySelector('.end-time');
	const startTime = startTimeInput.value;
	const endTime = endTimeInput.value;
	
	// Check if both times are set and start time is before end time
	if (startTime && endTime && startTime < endTime) {
		startTimeInput.classList.remove('invalid');
		endTimeInput.classList.remove('invalid');
	} else {
		// If the same input is being edited, don't mark it as invalid yet
		if ((input === startTimeInput && !endTime) || (input === endTimeInput && !startTime)) {
			input.classList.remove('invalid');
		} else {
			input.classList.add('invalid');
		}
	}
}

function isScheduleValidForDay(day) {
	const slots = document.querySelectorAll(`#${day}-slots .time-slot`);
	let isValid = false;
	
	slots.forEach(slot => {
		const times = slot.querySelectorAll('input[type="time"]');
		const startTime = times[0].value;
		const endTime = times[1].value;
		if (startTime && endTime && startTime < endTime) {
			isValid = true;
		} else {
			isValid = false;
		}
	});
	
	return isValid;
}

function updateCopyButtonVisibility(day) {
	const copyButton = document.querySelector(`#${day} button[data-action='copy']`);
	if (isScheduleValidForDay(day)) {
		copyButton.style.display = 'inline-block'; // Show button
	} else {
		copyButton.style.display = 'none'; // Hide button
	}
}

function copySchedule(sourceDay) {
	const days = ['monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday', 'sunday'];
	let targetDay = prompt('Enter day name to copy this schedule to:');
	targetDay = targetDay.toLowerCase();
	
	if (days.includes(targetDay)) {
		const sourceSlotsContainer = document.getElementById(sourceDay + '-slots');
		const targetSlotsContainer = document.getElementById(targetDay + '-slots');
		
		// Clear existing slots in target day
		targetSlotsContainer.innerHTML = '';
		
		// Clone each slot from source day to target day, including values
		const sourceSlots = sourceSlotsContainer.querySelectorAll('.time-slot');
		sourceSlots.forEach(slot => {
			const clonedSlot = slot.cloneNode(true);
			
			// Now, let's explicitly set the input values
			const sourceInputs = slot.querySelectorAll('input[type="time"]');
			const clonedInputs = clonedSlot.querySelectorAll('input[type="time"]');
			for (let i = 0; i < sourceInputs.length; i++) {
				clonedInputs[i].value = sourceInputs[i].value;
			}
			
			// Append cloned slot to target day
			targetSlotsContainer.appendChild(clonedSlot);
		});
		updateCopyButtonVisibility(targetDay);
	} else {
		alert('Invalid day name. Please try again.');
	}
}

document.getElementById('saveSchedule').addEventListener('click', saveSchedule);

function saveSchedule() {
	const schedule = {};
	
	days.forEach(day => {
		const slots = [];
		const daySlots = document.querySelectorAll(`#${day}-slots .time-slot`);
		daySlots.forEach(slot => {
			const times = slot.querySelectorAll('input[type="time"]');
			if (times.length === 2) {
				slots.push({start: times[0].value, end: times[1].value});
			}
		});
		schedule[day] = slots;
	});
	
	// Send AJAX request to backend
	const xhr = new XMLHttpRequest();
	xhr.open("POST", "/setConfig", true);
	xhr.setRequestHeader("Content-Type", "application/json");
	xhr.onreadystatechange = function () {
		if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {
			alert("Schedule saved successfully!");
		}
	};
	xhr.send(JSON.stringify(schedule));
}

function loadSchedule() {
	fetch('/getConfig') // Adjust the URL to your actual API endpoint
	.then(response => response.json())
	.then(scheduleJSON => {
		loadScheduleFromJSON(JSON.stringify(scheduleJSON));
	})
	.catch(error => {
		console.error('Failed to fetch schedule from Server:', error);
		// Handle failure, perhaps leave the default schedule in place
	});
}

function getPosixString(timezone) {
    const now = moment();
    const year = now.year();
    const startOfYear = moment.tz(`${year}-01-01T00:00:00`, timezone);
    const endOfYear = moment.tz(`${year}-12-31T23:59:59`, timezone);
    
	function getPosixTransitionRule(timestamp) {
        const dt = moment.tz(timestamp, timezone);
        const month = dt.month() + 1; // month() is zero-indexed, +1 for POSIX format
        const dayOfWeek = dt.day(); // day() gives 0 for Sunday, which matches POSIX requirements
        const nthWeek = Math.floor((dt.date() - 1) / 7) + 1;

        return `M${month}.${nthWeek}.${dayOfWeek}`;
    }

    // Find transitions within the current year
    let transitions = moment.tz.zone(timezone).untils;
    let format = moment.tz.zone(timezone).formats;
    let abbrs = moment.tz.zone(timezone).abbrs;

    // Search for the first and last transitions of the year
    let startIndex = transitions.findIndex(time => time >= startOfYear.valueOf());
    let endIndex = transitions.findIndex(time => time >= endOfYear.valueOf());

    if (startIndex < 0 || endIndex < 0) {
        console.log("No transitions found within this year, timezone might not observe DST");
        return `${abbrs[0]}${new Date(transitions[0]).getUTCFullYear() - 1}`;
    }

    let dstStart = transitions[startIndex];
    let dstEnd = transitions[startIndex + 1];

    // Create POSIX string
    let stdOffset = -startOfYear.utcOffset() / 60;
	let dstStartRule = getPosixTransitionRule(dstStart);
    let dstEndRule = getPosixTransitionRule(dstEnd);

    let stdAbbr = abbrs[startIndex];
    let dstAbbr = abbrs[startIndex + 1];
    let posixString = `${stdAbbr}${stdOffset}${dstAbbr},` + dstStartRule + "," + dstEndRule;

    return posixString;
}

// Example of initiating the process on page load
document.addEventListener('DOMContentLoaded', () => {
	var tz = Intl.DateTimeFormat().resolvedOptions().timeZone;
	var posix_tz = getPosixString(tz);
	//alert(posix_tz);
	var img = new Image();
	img.src = "/timezone.png?tz=" + posix_tz; // Send offset as query parameter
	img.style.display = 'none'; // Hide the image, it's not intended to be seen
	document.body.appendChild(img);

	const defaultScheduleJSON = `{
  "monday": [{"start": "07:00", "end": "09:00"}, {"start": "18:00", "end": "21:00"}],
  "tuesday": [{"start": "07:00", "end": "09:00"}, {"start": "18:00", "end": "21:00"}],
  "wednesday": [{"start": "07:00", "end": "09:00"}, {"start": "18:00", "end": "21:00"}],
  "thursday": [{"start": "07:00", "end": "09:00"}, {"start": "18:00", "end": "21:00"}],
  "friday": [{"start": "07:00", "end": "09:00"}, {"start": "18:00", "end": "21:00"}],
  "saturday": [{"start": "07:00", "end": "09:00"}, {"start": "18:00", "end": "21:00"}],
  "sunday": [{"start": "07:00", "end": "09:00"}, {"start": "18:00", "end": "21:00"}]
}`;
	loadScheduleFromJSON(defaultScheduleJSON);
	loadSchedule();
});
