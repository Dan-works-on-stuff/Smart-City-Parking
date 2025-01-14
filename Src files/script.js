async function fetchParkingData() {
    try {
        const response = await fetch("parking.txt");
        if (!response.ok) throw new Error("Failed to fetch parking data");
        const data = await response.text();
        parseParkingData(data);
    } catch (error) {
        console.error(error);
    }
}

function parseParkingData(data) {
    const container = document.getElementById("parking-container");
    container.innerHTML = ""; // Clear old data

    const floors = data.split("-------------------------").filter(Boolean);
    floors.forEach(floorData => {
        const floorLines = floorData.trim().split("\n");
        const letterMatch = floorLines[0].match(/Letter: (\w)/);
        const spotsMatch = floorLines[1].match(/Parking Spots: ([01\s]+)/);

        if (letterMatch && spotsMatch) {
            const floorLetter = letterMatch[1];
            const spots = spotsMatch[1].trim().split(" ");

            const floorDiv = document.createElement("div");
            floorDiv.classList.add("floor");

            const title = document.createElement("h2");
            title.textContent = `Floor ${floorLetter}`;
            floorDiv.appendChild(title);

            const spotsDiv = document.createElement("div");
            spotsDiv.classList.add("spots");

            spots.forEach((spot, index) => {
                const spotDiv = document.createElement("div");
                spotDiv.classList.add("spot", spot === "1" ? "busy" : "free");
                spotDiv.title = `Spot ${index + 1}: ${spot === "1" ? "Occupied" : "Free"}`;
                spotsDiv.appendChild(spotDiv);
            });

            floorDiv.appendChild(spotsDiv);
            container.appendChild(floorDiv);
        }
    });
}

// Update parking data every 10 seconds
fetchParkingData();
setInterval(fetchParkingData, 5000);