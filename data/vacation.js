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

window.onload = function() {
  loadCurrentVacationPeriod();
};

function getLocalISODateTime() {
    const now = new Date();
    const year = now.getFullYear();
    const month = (now.getMonth() + 1).toString().padStart(2, '0');
    const day = now.getDate().toString().padStart(2, '0');
    const hours = now.getHours().toString().padStart(2, '0');
    const minutes = now.getMinutes().toString().padStart(2, '0');
    
    return `${year}-${month}-${day} ${hours}:${minutes}`;
}

function loadCurrentVacationPeriod() {
    fetch('/getVacationPeriod')
    .then(response => response.json())
    .then(data => {
        console.log(data);
        if (data.startDate) {
            const formattedStartDate = formatDateTime(data.startDate);
            const formattedEndDate = data.endDate ? formatDateTime(data.endDate) : 'indefinite';
            
            document.getElementById('startDate').value = data.startDate;
            document.getElementById('endDate').value = data.endDate || '';
            document.getElementById('currentVacationPeriod').innerText = `Current Vacation Period is set from ${formattedStartDate} to ${formattedEndDate}`;
        } else {
            resetVacationModeSettings();
        }
    })
    .catch(error => {
        console.error('Error loading vacation period:', error);
        resetVacationModeSettings();
    });
}

function formatDateTime(isoString) {
    return isoString.replace('T', ' ');  // Replace 'T' with a space
}

function resetVacationModeSettings() {
    // Set the default start date to the current date and time
    const now = getLocalISODateTime();
    document.getElementById('startDate').value = now.slice(0, 16); // trim off seconds for compatibility
    document.getElementById('currentVacationPeriod').innerText = 'Current Vacation Period: Not set';
}

function clearVacationModeSettings() {
    resetVacationModeSettings();
    const data = {
        startDate: null,
        endDate: null
    };
    sendData('/setVacationPeriod', data);
    alert('Vacation period cleared successfully!');

}

function submitVacationModeSettings() {
    const startDate = document.getElementById('startDate').value;
    const endDate = document.getElementById('endDate').value;

    // Validate the end date
    if (endDate && new Date(startDate) >= new Date(endDate)) {
        alert('End date must be after the start date.');
        return;
    }

    // Prepare data to be sent to the server
    const data = {
        startDate: startDate,
        endDate: endDate || null
    };

    sendData('/setVacationPeriod', data);
    const formattedStartDate = formatDateTime(data.startDate);
    const formattedEndDate = data.endDate ? formatDateTime(data.endDate) : 'indefinite';
            
    document.getElementById('currentVacationPeriod').innerText = `Current Vacation Period:  ${formattedStartDate} to ${formattedEndDate}`;
    alert('Settings applied successfully!');
}

function sendData(url, data) {
    console.log('Sending:', data);
    fetch(url, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data)
    })
    .then(response => response.json())
    .then(data => {
        console.log('Success:', data);
        loadCurrentVacationPeriod();  // Reload the current setting after update
    })
    .catch((error) => {
        console.error('Error:', error);
    });
}
