<template>
  <v-container>
    <v-row justify="center">
        <v-col cols=5>
            <div class="ma-auto">
                Select an image from your PC to upload. Once uploaded, you can start the update process
            </div>
        </v-col>
    </v-row>
    <v-row justify="center">
      <v-col cols=2 >
        <v-file-input
            v-model="file"
            label="File input"
            outlined
            dense
            :show-size="1000"
        ></v-file-input>
        </v-col>
        <v-col cols=1>
            <v-btn
            :loading="uploading"
            :disabled="uploading"
            @click="upload()">
            Upload</v-btn>
        </v-col>
      <v-col cols=1>
          <v-btn
          :loading="flashing"
          :disabled="disabled"
          @click="flash()">
          Start Flashing</v-btn></v-col>
      <v-col cols=1></v-col>
    </v-row>
    <v-row justify="center">
        <v-col cols=5>
            <div class="ma-auto">
                {{ status }}
            </div>
        </v-col>
    </v-row>
  </v-container>
</template>

<script>

export default {
  data () {
    return {
        uploading: false,
        flashing: false,
        start_flash_btn: true,
        file: null,
        status: null
    }
  },
  computed: {
      disabled() {
          return this.start_flash_btn;
      }
  },
  methods: {
      upload: function() {
          this.uploading = !this.uploading;
          var reader = new FileReader();
          if(!this.file) {
              this.status = "No file selected"
              this.uploading = !this.uploading;
              return;
          }
          reader.readAsArrayBuffer(this.file);
          reader.onload = () => {
            this.$ajax
            .post('api/v1/ota/upload', reader.result)
            .then(res => {
                this.status = "Image upload successfull";
                this.start_flash_btn = !this.start_flash_btn;
            })
            .catch(error => {
                this.status = "Could not upload image - Statuscode: " + error.response.status
                this.uploading = !this.uploading;
            });
          }
      },
      flash: function() {
          this.flashing = !this.flashing
          this.start_flash_btn = !this.start_flash_btn
      }
  }
}
</script>