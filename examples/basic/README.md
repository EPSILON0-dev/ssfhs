# SSFHS - Basic Setup

This directory contains a basic example setup for the Simple Static File HTTP Server (SSFHS). It expands on the minimal setup by including additional error pages, styling, and client-side scripts to demonstrate more features and common use cases.

## Contained files:
* **README.md** - You're reading it right now.
* **index.html** - A minimal test page showcasing how the server handles various HTTP status scenarios and provides interactive buttons to test malformed requests.
* **style.css** - Styling for the test pages and error pages, providing a consistent and visually appealing look.
* **scripts.js** - JavaScript code for the test page, implementing buttons to perform various malformed requests and log the results.
* **400.html** - Custom page to display when the server responds with a 400 Bad Request status.
* **403.html** - Custom page for 403 Forbidden responses, shown when access is denied.
* **404.html** - Custom page for 404 Not Found errors, displayed when a requested resource does not exist.
* **forbidden.html** - A placeholder file to demonstrate server blocking, leading to a 403 response if accessed.
* **favicon.ico** - The favicon used for the pages.
* **ssfhs.conf** - Configuration file specifying protected files, custom error pages, and the index page.

## Additional info:
- The **ssfhs.conf** file protects important files like itself, the README, and the forbidden file from being served to users.
- Custom error pages for 400, 403, and 404 statuses provide user-friendly messages and consistent styling.
- The **index.html** page allows testing of how the server handles missing files, forbidden access, and malformed requests, with results logged dynamically.
- The design language uses a glassmorphism style with consistent colors and animations defined in **style.css**.
