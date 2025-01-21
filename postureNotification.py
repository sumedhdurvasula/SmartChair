from flask import Flask, request, jsonify, render_template_string

app = Flask(__name__)

current_posture_status = "unknown"

@app.route('/posture', methods=['POST'])
def posture():
    global current_posture_status
    try:
        data = request.get_json()
        posture_state = data.get('postureState')

        if posture_state is None:
            return jsonify({"error": "postureState not provided"}), 400

        if posture_state == 0:
            posture_status = "neutral"
        elif posture_state == 1:
            posture_status = "bad"
        elif posture_state == 2:
            posture_status = "good"
        else:
            posture_status = "unknown"

        current_posture_status = posture_status

        return jsonify({"postureStatus": posture_status}), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/')
def index():
    html_content = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Current Posture Status</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                text-align: center;
                margin-top: 50px;
            }
            #status {
                font-weight: bold;
                color: gray;
                font-size: 24px;
            }
            .good {
                color: green;
            }
            .bad {
                color: red;
            }
            .neutral {
                color: orange;
            }
            .unknown {
                color: gray;
            }
            /* Popup container */
            .popup {
                position: fixed;
                top: 20%;
                left: 50%;
                transform: translate(-50%, -50%);
                background-color: #f44336;
                color: white;
                padding: 20px;
                border-radius: 5px;
                display: none;
                z-index: 1000;
            }
            /* Close button */
            .popup .close-btn {
                margin-top: 10px;
                padding: 5px 10px;
                background-color: white;
                color: #f44336;
                border: none;
                border-radius: 3px;
                cursor: pointer;
            }
        </style>
        <script>
            let badPostureCount = 0;

            function fetchPosture() {
                fetch('/current_posture')
                    .then(response => response.json())
                    .then(data => {
                        const statusElement = document.getElementById('status');
                        statusElement.innerText = data.postureStatus;

                        statusElement.className = '';
                        if (data.postureStatus === 'good') {
                            statusElement.classList.add('good');
                            badPostureCount = 0;
                        } else if (data.postureStatus === 'bad') {
                            statusElement.classList.add('bad');
                            badPostureCount += 1;
                            if (badPostureCount >= 2) {
                                showPopup();
                                badPostureCount = 0; // Reset after notification
                            }
                        } else if (data.postureStatus === 'neutral') {
                            statusElement.classList.add('neutral');
                            badPostureCount = 0;
                        } else {
                            statusElement.classList.add('unknown');
                            badPostureCount = 0;
                        }
                    })
                    .catch(error => console.error('Error fetching posture:', error));
            }

            function showPopup() {
                const popup = document.getElementById('popup');
                popup.style.display = 'block';
            }

            function closePopup() {
                const popup = document.getElementById('popup');
                popup.style.display = 'none';
            }

            setInterval(fetchPosture, 2000);
            window.onload = fetchPosture;
        </script>
    </head>
    <body>
        <h1>Current Posture Status: <span id="status">unknown</span></h1>

        <div id="popup" class="popup">
            <p>Alert: Two consecutive bad postures detected. Please adjust your posture.</p>
            <button class="close-btn" onclick="closePopup()">Close</button>
        </div>
    </body>
    </html>
    """
    return render_template_string(html_content)

@app.route('/current_posture', methods=['GET'])
def current_posture():
    return jsonify({"postureStatus": current_posture_status}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
