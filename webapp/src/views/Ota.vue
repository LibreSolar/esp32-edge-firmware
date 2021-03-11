<template>
  <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex class="text-center">
        <v-card max-width="600" class="mx-auto my-auto">
          <v-card-title class="justify-center"
            >Over-the-Air-Upgrade</v-card-title
          >
          <v-card-text>
            <v-row justify="center" dense>
              <v-col cols="10">
                <div class="ma-auto">
                  Select an image from your PC to upload. Once uploaded, you can
                  start the update process
                </div>
              </v-col>
            </v-row>
            <v-row justify="center" dense>
              <v-col cols="10">
                <div class="ma-auto">Current Version: {{ oldFwVersion }}</div>
              </v-col>
            </v-row>
            <v-row justify="center">
              <v-col cols="10">
                <v-file-input
                  v-model="file"
                  label="File input"
                  outlined
                  dense
                  :show-size="1000"
                ></v-file-input>
              </v-col>
            </v-row>
            <v-row justify="center" dense>
              <v-spacer></v-spacer>
              <v-col cols="2">
                <v-btn
                  :loading="uploading"
                  :disabled="uploadDisabled"
                  @click="upload()"
                >
                  Upload</v-btn
                >
              </v-col>
              <v-spacer></v-spacer>
              <v-col cols="2">
                <v-btn
                  :loading="flashing"
                  :disabled="flashDisabled"
                  @click="flash()"
                >
                  Flash</v-btn
                ></v-col
              >
              <v-spacer></v-spacer>
            </v-row>
            <v-row justify="center">
              <v-alert
                v-model="alert"
                dense
                outlined
                type="error"
                transition="scale-transition"
                dismissible
                >{{ status }}</v-alert
              >
              <v-alert
                v-model="success"
                dense
                outlined
                type="success"
                transition="scale-transition"
                dismissible
                >{{ status }}</v-alert
              >
            </v-row>
          </v-card-text>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {
  data() {
    return {
      uploading: false,
      flashing: false,
      disableFlashBtn: true,
      disableUploadBtn: false,
      file: null,
      status: null,
      alert: false,
      success: false,
      oldFwVersion: null,
      NewFwVersion: null,
    };
  },
  mounted() {
    this.fetchData("old");
  },
  computed: {
    flashDisabled() {
      return this.disableFlashBtn;
    },
    uploadDisabled() {
      return this.disableUploadBtn;
    },
  },
  methods: {
    upload: function () {
      this.uploading = !this.uploading;
      this.disable("upload");
      var reader = new FileReader();
      if (!this.file) {
        this.status = "No file selected";
        this.showAlert();
        this.uploading = !this.uploading;
        this.enable("upload");
        return;
      }
      reader.readAsArrayBuffer(this.file);
      reader.onload = () => {
        this.$ajax
          .post("api/v1/ota/upload", reader.result)
          .then((res) => {
            this.status = "Image upload successfull";
            this.enable("flash");
            this.enable("upload");
            this.uploading = !this.uploading;
            this.showSuccess();
          })
          .catch((error) => {
            this.status =
              "Could not upload image - Statuscode: " + error.response.status;
            this.uploading = !this.uploading;
            this.enable("upload");
            this.disable("flash");
            this.showAlert();
          });
      };
    },
    flash: function () {
      this.flashing = !this.flashing;
      this.disable("flash");
      let id = this.$store.state.activeDeviceId;
      this.$ajax
        .get("api/v1/ota/" + id)
        .then((res) => {
          this.disable("flash");
          this.enable("upload");
          this.flashing = !this.flashing;
          this.fetchData("new");
        })
        .catch((error) => {
          this.status =
            "Could not be flashed: " +
            error.response.status +
            "-" +
            error.response.data;
          this.flashing = !this.flashing;
          this.enable("flash");
          this.enable("upload");
          this.showAlert();
        });
    },
    fetchData: function (target) {
      let id = this.$store.state.activeDeviceId;
      this.$ajax
        .get("api/v1/ts/" + id + "/info")
        .then((res) => {
          if (target == "old") {
            this.oldFwVersion = res.data.FirmwareVersion;
          } else if (target == "new") {
            this.NewFwVersion = res.data.FirmwareVersion;
            this.status =
              "Updated successfully from " +
              this.oldFwVersion +
              " --> " +
              this.NewFwVersion;
            this.showSuccess();
          }
          return;
        })
        .catch((error) => {
          this.status =
            "Device Information could not be fetched: " +
            error.response.status +
            "-" +
            error.response.data;
          this.showAlert();
          return;
        });
    },
    enable: function (btn) {
      if (btn == "flash") {
        this.disableFlashBtn = false;
      } else if (btn == "upload") {
        this.disableUploadBtn = false;
      }
    },
    disable: function (btn) {
      if (btn == "flash") {
        this.disableFlashBtn = true;
      } else if (btn == "upload") {
        this.disableUploadBtn = true;
      }
    },
    showAlert: function () {
      this.success = false;
      this.alert = true;
    },
    showSuccess: function () {
      this.alert = false;
      this.success = true;
    },
  },
};
</script>
