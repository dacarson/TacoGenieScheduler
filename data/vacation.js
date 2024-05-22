window.onload = function() {
  loadCurrentVacationPeriod();
};

function loadCurrentVacationPeriod() {
    fetch('/getCurrentVacationPeriod')
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
            // Set the default start date to the current date and time
            const now = new Date().toISOString();
            document.getElementById('startDate').value = now.slice(0, 16); // trim off seconds for compatibility
            document.getElementById('currentVacationPeriod').innerText = 'No Vacation Period is currently set.';
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
    const now = new Date().toISOString();
    document.getElementById('startDate').value = now.slice(0, 16); // trim off seconds for compatibility
    document.getElementById('currentVacationPeriod').innerText = 'Current Vacation Period: Not set';
}

function clearVacationModeSettings() {
    resetVacationModeSettings();
    sendData('/clearVacationPeriod', {});
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
