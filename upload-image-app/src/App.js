import "./App.css";
import { useState } from "react";

import { FileUploader } from "@aws-amplify/ui-react-storage";
import "@aws-amplify/ui-react/styles.css";

function App() {
  const [lastUploadedKey, setLastUploadedKey] = useState("");
  const [error, setError] = useState("");

  return (
    <div className="App">
      <header className="App-header" style={{ padding: 24 }}>
        <h2 style={{ marginBottom: 12 }}>Upload an image</h2>

        <FileUploader
          acceptedFileTypes={["image/*"]}
          path="public/"
          maxFileCount={1}
          isResumable
          onUploadStart={() => {
            setError("");
            setLastUploadedKey("");
          }}
          onUploadSuccess={({ key }) => {
            setLastUploadedKey(key);
          }}
          onUploadError={(err) => {
            setError(typeof err === "string" ? err : "Upload failed");
          }}
        />

        {lastUploadedKey ? (
          <p style={{ marginTop: 16 }}>
            Uploaded to S3 key: <code>{lastUploadedKey}</code>
          </p>
        ) : null}

        {error ? (
          <p style={{ marginTop: 16, color: "salmon" }}>
            {error}
          </p>
        ) : null}
      </header>
    </div>
  );
}

export default App;
